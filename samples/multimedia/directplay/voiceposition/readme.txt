//-----------------------------------------------------------------------------
// 
// Sample Name: VoicePosition Sample
// 
// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved.
// 
//-----------------------------------------------------------------------------


Description
===========
  VoicePosition shows how use 3D positioning with DirectPlay Voice.
  It uses a simple 2D grid to represent a playing field. Players may
  move around the playing field to hear the effects of 3D spatialization.

Path
====
  Source: Mssdk\Samples\Multimedia\DirectPlay\VoicePosition 

  Executable: Mssdk\Samples\Multimedia\DirectPlay\Bin

User's Guide
============
  Refer to User's Guide section of the SimplePeer sample for information
  on how to connect.  After connecting, the host will be presented with a 
  dialog to select the voice codec to use.  Typical voice applications will
  what automatically select one, or present this to the user in some other 
  fashion.  Once the chat session has begin, any client may alter the playback
  or capture settings by clicking on "Setup".  By click on the 2D grid, the
  player positions their avatar.  This in effect repositions the listener 
  object on the local client, and causes the remote clients to reposition 
  the voice buffer for the local client.

Programming Notes
=================
  The VoiceConnect sample is very similar in form to the SimplePeer as well as the 
  VoiceConnect sample.  
  
  For detailed programming notes on the DirectPlay basics this sample, refer 
  to Programming Notes section of the SimplePeer sample.  
  
  For detailed programming notes on the DirectPlay Voice basics 
  of this sample, refer to Programming Notes section of the VoiceConnect sample.  
  
  The VoicePosition differs from VoiceConnect by maintaining 3D sound buffers for each
  remote client, and updating the position of each as the remote players move on the 
  2D grid.   Since there are a number of threads accessing the same shared data, we need
  to be very careful.  This sample uses ref counting and critical sections to ensure 
  thread safety.  Here's how this is done:
  
  * Init DirectSound during the dialog init. See InitDirectSound()
        1. Call DirectSoundCreate() with DSDEVID_DefaultVoicePlayback to create a 
           DirectSound object using the default voice playback device.
        2. Call IDirectSound::SetCooperativeLevel() to set the DirectSound priority.
        3. Call IDirectSound::CreateSoundBuffer() with a DSBUFFERDESC containing 
           DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D to create a primary buffer with
           3D control.
        4. Query the primary buffer for the listener object, IDirectSound3DListener.
        5. Release the primary buffer since we don't need it anymore.
        6. Call IDirectSound3DListener::SetAllParameters to init all the parameters
           of the listener.
           
  * Init DirectPlay Voice using the DirectSound object we created. See WM_INITDIALOG in VoiceDlgProc().
        - We simply pass the g_pDS into CNetVoice::Init() which passes it to DirectPlay
          during the IDirectPlayVoiceClient::Connect call in the DVSOUNDDEVICECONFIG struct.           
  
  * Upon DPN_MSGID_CREATE_PLAYER. See DirectPlayMessageHandler()
        - We create a new player struct, APP_PLAYER_INFO, for this player and 
          stores it in the player's context.  This is similar to the VoiceConnect sample.
          This player struct stores the position of the client, and will be updated 
          as the player moves around.  All clients start in the center of the map.
           
  * Upon DVMSGID_CREATEVOICEPLAYER. See DirectPlayVoiceClientMessageHandler()
        1. We get the player context for this player.  The DPN_MSGID_CREATE_PLAYER will
           have already been process so if the player context can't be retrieved then the
           player has already left the session.  
        2. Addref the player's struct so that the voice layer keeps a ref count also.
        3. Check for DVPLAYERCAPS_HALFDUPLEX to see if the player is running in half duplex mode.
           If so, then the player can only listen but not talk.
        4. If this is not the local player, then do the following:
            - Enter the player's critical section since we are about to change data 
              in the player struct.  This ensures thread safety.
            - Call IDirectPlayVoiceClient::Create3DSoundBuffer() passing in the DPNID of the client, 
              to create a 3D DirectSound buffer associated with this client.
            - Call IDirectSound3DBuffer::SetMinDistance(), SetMaxDistance(), SetPosition(), and
              SetVelocity() to initialize the 3D sound buffer's settings.
            - Leave the player's critical section.
            - Call IDirectSound3DListener::CommitDeferredSettings() to commit any defered settings.
        5. Sets the DirectPlay Voice context for this player to be the player's struct -- 
           which is the same as the DirectPlay context.
        
  * Upon mouse click on 2D grid or position slider move:  See VoiceDlgProc().
        1. We get the mouse position in the client rect of the grid.
        2. We notify the other clients of this change by sending a GAMEMSG_PLAYERMOVED 
           packet to all of the other clients. See SendLocalPosition().
    
  * Upon DPN_MSGID_RECEIVE. See DirectPlayMessageHandler()
        1. Casts the pReceiveMsg->pReceiveData into a GAMEMSG_GENERIC* to 
           detrimine the packet type.
        2. If the packet is of type GAME_MSGID_PLAYERMOVED, it casts the 
           pReceiveMsg->pReceiveData to a GAMEMSG_PLAYERMOVED struct.
        3. We cast the pReceiveMsg->pvPlayerContext into a APP_PLAYER_INFO*.  
        4. We enter the player's critical section since we are about to modify the struct.  
        5. Update the player's position in the struct.
        6. Leave the critical section.
        7. Post a message to the dialog thread to update the grid.  This will keep the 
           DirectPlay message handler from blocking.
        
  * Upon updating the grid.  See UpdateGrid()
        1. It enums the players by calling IDirectPlay8Peer::EnumPlayersAndGroups().
        2. For each player, it enters the player context critical section and it 
           tries to get the player context.  This may fail since the
           player may have already left the session.
        3. If it succeeded then, it casts the context to the player stuct, 
           addrefs the struct, and leaves the critical section.
        4. It then enter's the player's critical section since we are about 
           to access the player's data.  This ensures thread safety.
        5. If the player is the local player, then it calls 
           IDirectSound3DListener::SetPosition() otherwise it calls 
           IDirectSound3DBuffer::SetPosition() on the player's 3D sound buffer.
        6. To cleanup, we leave the player's critical section, and release the 
           ref count from the player's struct.
        
  * Upon DVMSGID_DELETEVOICEPLAYER.  See DirectPlayVoiceClientMessageHandler()
        1. It casts the DirectPlay Voice player context to a APP_PLAYER_INFO*.
        2. It enters the player's critical section since we are about to 
           read and modify the struct.
        3. It posts a message to the dialog thread to update the UI.  This keeps
           the DirectPlay Voice message handler from blocking.
        4. If its not the local player then it calls IDirectPlayVoiceClient::Delete3DSoundBuffer
           to delete the player's 3D sound buffer.
        5. It leaves the player's critical section.
        6. It release's the refcount on the player's struct.                  
  
  * Upon DPN_MSGID_DESTROY_PLAYER.  See DirectPlayMessageHandler()
        1. It releases the DirectPlay ref count on the player struct, and deletes
           the struct if its zero.  The struct may be in use by another thread so 
           it may not be delete right here, but when the ref count is reduced to zero
           the struct will be deleted.
        2. It posts a message to the dialog thread to update the UI.  This keeps
           the DirectPlay message handler from blocking.
           
  
           
