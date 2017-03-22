/*
 * Copyright (C) 2017 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <boost/pointer_cast.hpp>

#include "gazebo/common/Plugin.hh"
#include "gazebo/physics/ode/ODEPhysics.hh"

#include "WheelTrackedVehiclePlugin.hh"


using namespace gazebo;

GZ_REGISTER_MODEL_PLUGIN(WheelTrackedVehiclePlugin)

WheelTrackedVehiclePlugin::WheelTrackedVehiclePlugin()
{
}

WheelTrackedVehiclePlugin::~WheelTrackedVehiclePlugin()
{
}

void WheelTrackedVehiclePlugin::Load(physics::ModelPtr _model,
                                     sdf::ElementPtr _sdf)
{
  TrackedVehiclePlugin::Load(_model, _sdf);

  this->world = _model->GetWorld();

  this->LoadParam(_sdf, "max_torque", this->maxTorque, 5.0);

  // load the wheels
  for (auto trackPair : this->trackNames)
  {
    auto track(trackPair.first);
    const auto &trackName(trackPair.second);
    const auto jointTagName = trackName + "_joint";

    std::vector<std::string> jointNames;

    // read names of all wheel joints on one track
    if (_sdf->HasElement(jointTagName))
    {
      sdf::ElementPtr elem = _sdf->GetElement(jointTagName);
      while (elem)
      {
        jointNames.push_back(elem->Get<std::string>());
        elem = elem->GetNextElement(jointTagName);
      }
    }

    GZ_ASSERT(this->jointNames[track].size() >= 2,
              "WheelTrackedVehiclePlugin: At least two " + jointTagName +
                " tags have to be specified.");

    // find the corresponding joint and wheel radius
    for (auto jointName : jointNames)
    {
      LoadWheel(_model, track, jointName);
    }
  }

  gzdbg << "WheelTrackedVehiclePlugin loaded with " <<
    this->wheels[Tracks::LEFT].size() << " left wheels and " <<
    this->wheels[Tracks::RIGHT].size() << " right wheels." << std::endl;
}

void WheelTrackedVehiclePlugin::LoadWheel(physics::ModelPtr &_model,
    Tracks &_track, const std::string &_jointName)
{
  auto joint = _model->GetJoint(_jointName);

  GZ_ASSERT(joint, "WheelTrackedVehiclePlugin (ns = " +
        GetRobotNamespace() + ") couldn't get " + this->trackNames[_track] +
        " joint named \"" + _jointName + "\"");

  GZ_ASSERT(joint->GetType() == physics::Base::HINGE_JOINT, "Joint " +
        _jointName + " is not a hinge (revolute) joint.");

  auto childWheel = joint->GetChild();
  GZ_ASSERT(childWheel, "Joint " + _jointName + " has no child link that "
        " could act as the wheel.");

  if (childWheel->GetSelfCollide())
  {
    gzwarn << "Wheel " << childWheel->GetName() << " has autocollisions on. "
      "You should usually set <self_collide> in the SDF to 0." << std::endl;
  }

  // if the wheel is represented as a single cylinder, read radius from the
  // cylinder, otherwise use bounding box to determine the radius
  double radius;

  // needed to avoid ambiguous call to GetCollision()
  unsigned int zero = 0u;

  if (childWheel->GetCollisions().size() == 1 &&
      childWheel->GetCollision(zero)->GetShapeType() ==
        physics::Base::CYLINDER_SHAPE)
  {
    auto shape = boost::dynamic_pointer_cast<physics::CylinderShape>(
      childWheel->GetCollision(zero)->GetShape());
    GZ_ASSERT(shape, "Cannot cast shape to physics::CylinderShape");

    radius = shape->GetRadius();
  }
  else
  {
    // This assumes that the largest dimension of the wheel is the diameter
    auto bb = childWheel->BoundingBox();
    radius = bb.Size().Max() * 0.5;
  }

  WheelInfoPtr wheel = WheelInfoPtr(new WheelInfo);
  wheel->joint = joint;
  wheel->jointName = _jointName;
  wheel->radius = radius;

  wheels[_track].push_back(wheel);
}

void WheelTrackedVehiclePlugin::Init()
{
  TrackedVehiclePlugin::Init();

  for (auto trackPair : this->trackNames)
  {
    auto track(trackPair.first);

    for (auto wheel : this->wheels[track])
    {
      wheel->joint->SetParam("fmax", 0, this->maxTorque);
    }
  }

  // set "cone_model" friction model
  auto physics = this->world->Physics();
  if (physics->GetType() == "ode")
  {
    auto odePhysics = boost::dynamic_pointer_cast<physics::ODEPhysics>(physics);
    GZ_ASSERT(odePhysics, "Cannot cast to physics::ODEPhysics");

    if (odePhysics->GetFrictionModel() != "cone_model")
    {
      gzwarn << "WheelTrackedVehiclePlugin: Setting ODE friction model to "
        "cone_model from " << odePhysics->GetFrictionModel() << std::endl;
      odePhysics->SetFrictionModel("cone_model");
    }
  }
  else
  {
    gzwarn << "WheelTrackedVehiclePlugin: This plugin only works correctly "
      "with the cone_model friction model, which is currently available only "
      "in ODE. In other physics engines, you can expect the model to have "
      "difficulties when steering." << std::endl;
  }

  // set the desired friction to tracks (override the values set in the
  // SDF model)
  this->UpdateTrackSurface();
}

void WheelTrackedVehiclePlugin::Reset()
{
  std::lock_guard<std::mutex> lock(this->mutex);

  for (auto trackPair : this->trackNames)
  {
    auto track(trackPair.first);

    for (auto wheel : this->wheels[track])
    {
      wheel->joint->SetParam("vel", 0, 0.);
    }
  }

  TrackedVehiclePlugin::Reset();
}

void WheelTrackedVehiclePlugin::SetTrackVelocity(double _left, double _right)
{
  std::lock_guard<std::mutex> lock(this->mutex);

  this->trackVelocity[Tracks::LEFT] = _left;
  this->trackVelocity[Tracks::RIGHT] = _right;

  for (auto trackPair : this->trackNames)
  {
    auto track(trackPair.first);

    for (auto wheel : this->wheels[track])
    {
      double angularVelocity = this->trackVelocity[track] / wheel->radius;
      wheel->joint->SetParam("vel", 0, angularVelocity);
    }
  }
}

void WheelTrackedVehiclePlugin::UpdateTrackSurface()
{
  for (auto trackPair : this->trackNames)
  {
    auto track(trackPair.first);
    for (auto wheel : this->wheels[track])
    {
      auto wheelLink = wheel->joint->GetChild();
      this->SetLinkMu(wheelLink);
    }
  }
}
