/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003  
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * Desc: Controller base class.
 * Author: Nathan Koenig
 * Date: 01 Feb 2007
 * SVN: $Id$
 */
#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <string>

#include "UpdateParams.hh"

namespace gazebo
{
  class XMLConfigNode;
  class Iface;
  class Entity;

/// \addtogroup controllers
/// \{
/** \defgroup controller controller
\{ 
*/

/// \brief Base class for all controllers
class Controller
{
  /// \brief Constructor
  /// \param iface The libgazebo interface for the controller
  /// \param parent The parent entity, must be a Model or a Sensor
  public: Controller( Iface *iface, Entity *parent );

  /// \brief Destructor
  public: virtual ~Controller();

  /// \brief Load the controller. Called once on startup
  /// \param node The XMLConfig node pointer
  /// \return 0 on success
  public: int Load(XMLConfigNode *node);

  /// \brief Initialize the controller. Called once on startup.
  /// \return 0 on success
  public: int Init();

  /// \brief Update the controller. Called every cycle.
  /// \param params Parameters to the update cycle
  /// \return 0 on success
  public: int Update(UpdateParams &params);

  /// \brief Finialize the controller. Called once on completion.
  /// \return 0 on success
  public: int Fini();

  /// \brief Load function for the child class
  /// \return 0 on success
  protected: virtual int LoadChild(XMLConfigNode * /*node*/) {return 0;}

  /// \brief Init function for the child class
  /// \return 0 on success
  protected: virtual int InitChild() {return 0;}

  /// \brief Update function for the child class
  /// \return 0 on success
  protected: virtual int UpdateChild(UpdateParams &/*params*/) {return 0;}

  /// \brief Fini function for the child class
  /// \return 0 on success
  protected: virtual int FiniChild() {return 0;}

  /// \brief Return the name of this controller
  /// \return The name of the controller
  public: std::string GetName() const;

  /// \brief The controller's name
  protected: std::string name;

  /// \brief The entity that owns this controller
  protected: Entity *parent;

  protected: Iface *iface;

  /// \brief Update period 
  protected: double updatePeriod;
};

/** \} */
/// \}

}

#endif

