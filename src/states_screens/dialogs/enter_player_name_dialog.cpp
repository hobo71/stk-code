//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2013 Marianne Gagnon
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "states_screens/dialogs/enter_player_name_dialog.hpp"

#include <IGUIEnvironment.h>

#include "audio/sfx_manager.hpp"
#include "challenges/unlock_manager.hpp"
#include "config/player_manager.hpp"
#include "config/player_profile.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/widgets/button_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "guiengine/widgets/text_box_widget.hpp"
#include "states_screens/state_manager.hpp"
#include "utils/translation.hpp"
#include "utils/string_utils.hpp"


using namespace GUIEngine;
using namespace irr;
using namespace irr::core;
using namespace irr::gui;

// -----------------------------------------------------------------------------

EnterPlayerNameDialog::EnterPlayerNameDialog(INewPlayerListener* listener,
                                             const float w, const float h) :
        ModalDialog(w, h)
{
    m_listener = listener;
    m_self_destroy = false;
    loadFromFile("enter_player_name_dialog.stkgui");

    TextBoxWidget* text_field = getWidget<TextBoxWidget>("textfield");
    assert(text_field != NULL);
    text_field->setFocusForPlayer(PLAYER_ID_GAME_MASTER);

    std::string username = "";

    // If there is no player (i.e. first start of STK), try to pick
    // a good default name
    if (PlayerManager::get()->getNumPlayers() == 0)
    {
        if (getenv("USERNAME") != NULL)        // for windows
            username = getenv("USERNAME");
        else if (getenv("USER") != NULL)       // Linux, Macs
            username = getenv("USER");
        else if (getenv("LOGNAME") != NULL)    // Linux, Macs
            username = getenv("LOGNAME");
    }
    text_field->setText(username.c_str());
}   // EnterPlayerNameDialog

// -----------------------------------------------------------------------------

EnterPlayerNameDialog::~EnterPlayerNameDialog()
{
}   // ~EnterPlayerNameDialog

// -----------------------------------------------------------------------------
GUIEngine::EventPropagation 
            EnterPlayerNameDialog::processEvent(const std::string& eventSource)
{
    if (eventSource == "cancel")
    {
        dismiss();
        return GUIEngine::EVENT_BLOCK;
    }
    return GUIEngine::EVENT_LET;
}   // processEvent

// -----------------------------------------------------------------------------
void EnterPlayerNameDialog::onEnterPressedInternal()
{
    // ---- Cancel button pressed
    const int playerID = PLAYER_ID_GAME_MASTER;
    ButtonWidget* cancelButton = getWidget<ButtonWidget>("cancel");
    if (GUIEngine::isFocusedForPlayer(cancelButton, playerID))
    {
        std::string fakeEvent = "cancel";
        processEvent(fakeEvent);
        return;
    }

    // ---- Otherwise, see if we can accept the new name
    TextBoxWidget* text_field = getWidget<TextBoxWidget>("textfield");
    stringw player_name = text_field->getText().trim();
    if (StringUtils::notEmpty(player_name))
    {
        // check for duplicates
        const int amount = PlayerManager::get()->getNumPlayers();
        for (int n=0; n<amount; n++)
        {
            if (PlayerManager::get()->getPlayer(n)->getName() == player_name)
            {
                LabelWidget* label = getWidget<LabelWidget>("title");
                label->setText(_("Cannot add a player with this name."), false);
                sfx_manager->quickSound( "anvil" );
                return;
            }
        }

        // Finally, add the new player.
        PlayerManager::get()->addNewPlayer(player_name);
        PlayerManager::get()->save();

        // It's unsafe to delete from inside the event handler so we do it
        // in onUpdate (which checks for m_self_destroy)
        m_self_destroy = true;
    } // if valid name
    else
    {
        LabelWidget* label = getWidget<LabelWidget>("title");
        label->setText(_("Cannot add a player with this name."), false);
        sfx_manager->quickSound( "anvil" );
    }
}   // onEnterPressedInternal

// -----------------------------------------------------------------------------
void EnterPlayerNameDialog::onUpdate(float dt)
{
    // It's unsafe to delete from inside the event handler so we do it here
    if (m_self_destroy)
    {
        TextBoxWidget* text_field = getWidget<TextBoxWidget>("textfield");
        stringw player_name = text_field->getText().trim();

        // irrLicht is too stupid to remove focus from deleted widgets
        // so do it by hand
        GUIEngine::getGUIEnv()->removeFocus( text_field->getIrrlichtElement() );
        GUIEngine::getGUIEnv()->removeFocus( m_irrlicht_window );

        // we will destroy the dialog before notifying the listener to be safer.
        // but in order not to crash we must make a local copy of the listern
        // otherwise we will crash
        INewPlayerListener* listener = m_listener;

        ModalDialog::dismiss();

        if (listener != NULL) listener->onNewPlayerWithName( player_name );
    }
}   // onUpdate
