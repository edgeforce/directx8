Attribute VB_Name = "modDplay"
Option Explicit
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'
'  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
'
'  File:       modDPlay.bas
'
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

'Here are all of the messages we can transfer in this app
Public Enum vbMsgType
    MsgChat 'We are talking in the chat channel
    MsgWhisper 'We are whispering to someone in the chat channel
    MsgAskToJoin 'We want to ask if we can join this session
    MsgAcceptJoin 'Accept the call
    MsgRejectJoin 'Reject the call
    MsgCancelCall 'Cancel the call
    MsgShowChat 'Show the chat window
    MsgSendFileRequest 'Request a file transfer
    MsgSendFileAccept 'Accept the file transfer
    MsgSendFileDeny 'Deny the file transfer
    MsgSendFileInfo 'File information (size)
    MsgSendFilePart 'Send a chunk of the file
    MsgAckFilePart 'Acknowledge the file part
    MsgSendDrawPixel 'Send a drawn pixel
    MsgSendDrawLine 'Send a drawn line
    MsgShowWhiteBoard 'Show the whiteboard window
    MsgClearWhiteBoard 'Clear the contents of the whiteboard
End Enum

'Win32 declares
Public Declare Function GetUserName Lib "advapi32.dll" Alias "GetUserNameA" (ByVal lpBuffer As String, nSize As Long) As Long
Public Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)
Public Declare Function Shell_NotifyIcon Lib "shell32.dll" Alias "Shell_NotifyIconA" (ByVal dwMessage As Long, lpData As NOTIFYICONDATA) As Long

Public Type NOTIFYICONDATA
    cbSize As Long
    hwnd As Long
    uID As Long
    uFlags As Long
    uCallbackMessage As Long
    hIcon As Long
    sTip As String * 64
End Type
    
Public Const NIM_ADD = &H0
Public Const NIM_MODIFY = &H1
Public Const NIM_DELETE = &H2
Public Const NIF_MESSAGE = &H1
Public Const NIF_ICON = &H2
Public Const NIF_TIP = &H4
Public Const NIF_DOALL = NIF_MESSAGE Or NIF_ICON Or NIF_TIP
Public Const WM_MOUSEMOVE = &H200
Public Const WM_LBUTTONDBLCLK = &H203
Public Const WM_RBUTTONUP = &H205

'Constants
Public Const AppGuid = "{9073823A-A565-4865-87EC-19B93B014D27}"
Public Const glDefaultPort As Long = 9987

'DirectX variables
Public dx As DirectX8
Public dpp As DirectPlay8Peer
Public dvClient As DirectPlayVoiceClient8
Public dvServer As DirectPlayVoiceServer8

'Window variables for this app
Public ChatWindow As frmChat
Public WhiteBoardWindow As frmWhiteBoard
Public NetWorkForm As frmNetwork

'Misc app variables
Public sysIcon As NOTIFYICONDATA
Public gsUserName As String
Public glAsyncEnum As Long
Public glMyPlayerID As Long
Public gfHost As Boolean

Public Sub Main()
    If App.PrevInstance Then
        'We can only run one instance of this sample per machine since we
        'specify a port to run this application on.  Only one application can
        'be listening (hosting) on a particular port at any given time.
        MsgBox "Only one instance of vbConferencer may be run at a time.", vbOKOnly Or vbInformation, "Only one"
        Exit Sub
    End If
    'Set our username up
    gsUserName = GetSetting("VBDirectPlay", "Defaults", "UserName", vbNullString)
    If gsUserName = vbNullString Then
        'If there is not a default username, then pick the currently
        'logged on username
        gsUserName = Space$(255)
        GetUserName gsUserName, 255
        gsUserName = Left$(gsUserName, InStr(gsUserName, Chr$(0)) - 1)
    End If
    Screen.MousePointer = vbHourglass
    'Show the splash screen
    frmSplash.Show
    'Start the host
    Set NetWorkForm = New frmNetwork
    Load NetWorkForm
    'We don't need it anymore
    Unload frmSplash
    Screen.MousePointer = vbNormal
    NetWorkForm.Show
End Sub

Public Sub InitDPlay()
    Set dx = New DirectX8
    Set dpp = dx.DirectPlayPeerCreate
End Sub

Public Sub Cleanup()
    On Error Resume Next
    Set ChatWindow = Nothing
    Set WhiteBoardWindow = Nothing
    If Not dpp Is Nothing Then dpp.UnRegisterMessageHandler 'Stop taking messages
    If Not (dvClient Is Nothing) Then dvClient.UnRegisterMessageHandler
    If Not (dvServer Is Nothing) Then dvServer.UnRegisterMessageHandler
    'Lets wait right here for a second letting things finish cleaning up
    Sleep 50
    DoEvents
    If Not (dvClient Is Nothing) Then dvClient.Disconnect DVFLAGS_SYNC
    If Not (dvServer Is Nothing) Then dvServer.StopSession 0
    'Close our peer connection
    If Not dpp Is Nothing Then dpp.Close
    'Destroy the objects
    Set dvClient = Nothing
    Set dvServer = Nothing
    'Lose references to peer and dx objects
    Set dpp = Nothing
    Set dx = Nothing
End Sub

Public Sub StartHosting(MsgForm As Form)
    Dim dpa As DirectPlay8Address
    Dim oPlayer As DPN_PLAYER_INFO
    Dim oAppDesc As DPN_APPLICATION_DESC
    
    'Make sure we're ready to host
    Cleanup
    InitDPlay
    NetWorkForm.cmdHangup.Enabled = False
    NetWorkForm.cmdCall.Enabled = True
    gfHost = True
    'Register the Message Handler
    dpp.RegisterMessageHandler MsgForm
    'Set the peer info
    oPlayer.lInfoFlags = DPNINFO_NAME
    oPlayer.Name = gsUserName
    dpp.SetPeerInfo oPlayer, DPNOP_SYNC
    'Create an address
    Set dpa = dx.DirectPlayAddressCreate
    'We will only be connecting via TCP/IP
    dpa.SetSP DP8SP_TCPIP
    dpa.AddComponentLong DPN_KEY_PORT, glDefaultPort
    
    'First set up our application description
    With oAppDesc
        .guidApplication = AppGuid
        .lMaxPlayers = 10 'We don't want to overcrowd our 'room'
        .lFlags = DPNSESSION_NODPNSVR
    End With
    'Start our host
    dpp.Host oAppDesc, dpa
    Set dpa = Nothing
        
    'After we've created the session and let's start
    'the DplayVoice server
    Dim oSession As DVSESSIONDESC

    'Create our DPlayVoice Server
    Set dvServer = dx.DirectPlayVoiceServerCreate

    'Set up the Session
    oSession.lBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT
    oSession.lBufferQuality = DVBUFFERQUALITY_DEFAULT
    oSession.lSessionType = DVSESSIONTYPE_PEER
    oSession.guidCT = vbNullString

    'Init and start the session
    dvServer.Initialize dpp, 0
    dvServer.StartSession oSession, 0
    ConnectVoice
    Set dpa = Nothing
End Sub

Public Sub Connect(MsgForm As Form, ByVal sHost As String)
    Dim dpa As DirectPlay8Address
    Dim dpl As DirectPlay8Address
    Dim oPlayer As DPN_PLAYER_INFO
    Dim oAppDesc As DPN_APPLICATION_DESC
    
    'Try to connect to the host
    'Make sure we're ready to connect
    Cleanup
    InitDPlay
    NetWorkForm.cmdCall.Enabled = False
    gfHost = False
    'Register the Message Handler
    dpp.RegisterMessageHandler MsgForm
    'Set the peer info
    oPlayer.lInfoFlags = DPNINFO_NAME
    oPlayer.Name = gsUserName
    dpp.SetPeerInfo oPlayer, DPNOP_SYNC
    'Now try to enum hosts
    
    'Create an address
    Set dpa = dx.DirectPlayAddressCreate
    'We will only be connecting via TCP/IP
    dpa.SetSP DP8SP_TCPIP
    dpa.AddComponentString DPN_KEY_HOSTNAME, sHost 'We only want to enumerate connections on this host
    dpa.AddComponentLong DPN_KEY_PORT, glDefaultPort
    
    Set dpl = dx.DirectPlayAddressCreate
    'We will only be connecting via TCP/IP
    dpl.SetSP DP8SP_TCPIP
    
    'First set up our application description
    With oAppDesc
        .guidApplication = AppGuid
    End With
    'Try to connect to this host
    dpp.Connect oAppDesc, dpa, dpl, 0, ByVal 0&, 0
    Set dpa = Nothing
    Set dpl = Nothing
End Sub

Public Sub ConnectVoice()
    Dim oSound As DVSOUNDDEVICECONFIG
    Dim oClient As DVCLIENTCONFIG
    'Now create a client as well (so we can both talk and listen)
    Set dvClient = dx.DirectPlayVoiceClientCreate
    'Now let's create a client event..
    dvClient.Initialize dpp, 0
    'Set up our client and sound structs
    oClient.lFlags = DVCLIENTCONFIG_AUTOVOICEACTIVATED Or DVCLIENTCONFIG_AUTORECORDVOLUME
    oClient.lBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT
    oClient.lBufferQuality = DVBUFFERQUALITY_DEFAULT
    oClient.lNotifyPeriod = 0
    oClient.lThreshold = DVTHRESHOLD_UNUSED
    oClient.lPlaybackVolume = DVPLAYBACKVOLUME_DEFAULT
    oSound.hwndAppWindow = NetWorkForm.hwnd
    
    On Error Resume Next
    'Connect the client
    dvClient.Connect oSound, oClient, 0
    If Err.Number = DVERR_RUN_SETUP Then    'The audio tests have not been run on this
                                            'machine.  Run them now.
        'we need to run setup first
        Dim dvSetup As DirectPlayVoiceTest8
        
        Set dvSetup = dx.DirectPlayVoiceTestCreate
        dvSetup.CheckAudioSetup vbNullString, vbNullString, NetWorkForm.hwnd, 0 'Check the default devices since that's what we'll be using
        Set dvSetup = Nothing
        dvClient.Connect oSound, oClient, 0
    ElseIf Err.Number <> 0 And Err.Number <> DVERR_PENDING Then
        MsgBox "Could not start DirectPlayVoice.  This sample will not have any voice capablities." & vbCrLf & "Error:" & CStr(Err.Number), vbOKOnly Or vbInformation, "No Voice"
        Exit Sub
    End If
    On Error GoTo 0
    Dim lTargets(0) As Long
    
    lTargets(0) = DVID_ALLPLAYERS
    On Error Resume Next
    'Connect the client
    dvClient.SetTransmitTargets lTargets, 0
    If Err.Number <> 0 And Err.Number <> DVERR_PENDING Then
        MsgBox "Could not start DirectPlayVoice.  This sample will not have any voice capablities." & vbCrLf & "Error:" & CStr(Err.Number), vbOKOnly Or vbInformation, "No Voice"
        Exit Sub
    End If
End Sub
