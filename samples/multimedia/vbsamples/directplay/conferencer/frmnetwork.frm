VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Begin VB.Form frmNetwork 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "vbConferencer"
   ClientHeight    =   4365
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   3930
   Icon            =   "frmNetwork.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4365
   ScaleWidth      =   3930
   StartUpPosition =   3  'Windows Default
   Begin MSComDlg.CommonDialog cdlSend 
      Left            =   6360
      Top             =   3180
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
      DialogTitle     =   "Send File"
      Filter          =   "Any File |*.*"
      Flags           =   4
      InitDir         =   "C:\"
   End
   Begin VB.Timer tmrJoin 
      Enabled         =   0   'False
      Interval        =   50
      Left            =   6420
      Top             =   540
   End
   Begin VB.Timer tmrUpdate 
      Enabled         =   0   'False
      Interval        =   7500
      Left            =   6420
      Top             =   60
   End
   Begin VB.TextBox txtCall 
      Height          =   285
      Left            =   60
      TabIndex        =   0
      Top             =   300
      Width           =   2535
   End
   Begin VB.ListBox lstUsers 
      Height          =   2595
      Left            =   60
      TabIndex        =   3
      Top             =   1020
      Width           =   3795
   End
   Begin VB.CommandButton cmdHangup 
      Height          =   495
      Left            =   3240
      MaskColor       =   &H00FF0000&
      Picture         =   "frmNetwork.frx":030A
      Style           =   1  'Graphical
      TabIndex        =   2
      ToolTipText     =   "Hang up"
      Top             =   120
      UseMaskColor    =   -1  'True
      Width           =   495
   End
   Begin VB.CommandButton cmdCall 
      Default         =   -1  'True
      Height          =   495
      Left            =   2700
      MaskColor       =   &H000000FF&
      Picture         =   "frmNetwork.frx":0A0C
      Style           =   1  'Graphical
      TabIndex        =   1
      ToolTipText     =   "Call a friend"
      Top             =   120
      UseMaskColor    =   -1  'True
      Width           =   495
   End
   Begin VB.CommandButton cmdWhiteBoard 
      Height          =   495
      Left            =   2325
      MaskColor       =   &H000000FF&
      Picture         =   "frmNetwork.frx":110E
      Style           =   1  'Graphical
      TabIndex        =   6
      ToolTipText     =   "Use the whiteboard"
      Top             =   3720
      UseMaskColor    =   -1  'True
      Width           =   495
   End
   Begin VB.CommandButton cmdChat 
      Height          =   495
      Left            =   1125
      MaskColor       =   &H000000FF&
      Picture         =   "frmNetwork.frx":1A18
      Style           =   1  'Graphical
      TabIndex        =   4
      ToolTipText     =   "Chat with someone"
      Top             =   3720
      UseMaskColor    =   -1  'True
      Width           =   495
   End
   Begin VB.CommandButton cmdSendFile 
      Height          =   495
      Left            =   1725
      MaskColor       =   &H000000FF&
      Picture         =   "frmNetwork.frx":2322
      Style           =   1  'Graphical
      TabIndex        =   5
      ToolTipText     =   "Transfer files to someone"
      Top             =   3720
      UseMaskColor    =   -1  'True
      Width           =   495
   End
   Begin VB.Label Label1 
      BackStyle       =   0  'Transparent
      Caption         =   "Enter a name or IP to call"
      Height          =   195
      Index           =   1
      Left            =   60
      TabIndex        =   8
      Top             =   60
      Width           =   2475
   End
   Begin VB.Label Label1 
      BackStyle       =   0  'Transparent
      Caption         =   "Users currently in this session"
      Height          =   315
      Index           =   0
      Left            =   60
      TabIndex        =   7
      Top             =   780
      Width           =   3735
   End
   Begin VB.Menu mnuPopup 
      Caption         =   "PopUp"
      Visible         =   0   'False
      Begin VB.Menu mnuExit 
         Caption         =   "E&xit"
      End
   End
End
Attribute VB_Name = "frmNetwork"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'
'  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
'
'  File:       frmNetwork.frm
'
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Implements DirectPlay8Event

Private Const mlFileChunkSize As Long = 512

Private msFile As String
Private mlFileSize As Long
Private mlSendToID As Long

Public msReceiveFile As String
Public mlReceiveFileSize As Long
Public mlReceiveSendToID As Long

Private moCallBack As DirectPlay8Event
Private mfExit As Boolean

Private Sub cmdCall_Click()
    If txtCall.Text = vbNullString Then
        MsgBox "You must type the name or address of the person you wish to call before I can make the call.", vbOKOnly Or vbInformation, "No callee"
        Exit Sub
    End If
    Connect Me, txtCall.Text
    
End Sub

Private Sub cmdChat_Click()
    If lstUsers.ListCount < 2 Then
        MsgBox "You must have at least two people in the session before you can chat.", vbOKOnly Or vbInformation, "Not enough people"
        Exit Sub
    End If
    If ChatWindow Is Nothing Then Set ChatWindow = New frmChat
    ChatWindow.Show vbModeless
    'Notify everyone
    SendOpenChatWindowMessage
    Set moCallBack = ChatWindow
End Sub

Private Sub cmdHangup_Click()
    'Cleanup and quit
    mfExit = True
    Unload Me
End Sub

Private Sub cmdSendFile_Click()
    Dim lMsg As Long, lOffset As Long
    Dim oBuf() As Byte
    
    If msFile <> vbNullString Then
        MsgBox "A previous file transfer is still ongoing, please wait for it to finish.", vbOKOnly Or vbInformation, "Wait"
        Exit Sub
    End If
    If lstUsers.ListIndex < 0 Then
        MsgBox "You must select someone to send a file to before sending one.", vbOKOnly Or vbInformation, "No selection"
        Exit Sub
    End If
    If lstUsers.ListIndex < 1 Then
        MsgBox "You must select someone other than yourself to send a file to before sending one.", vbOKOnly Or vbInformation, "No selection"
        Exit Sub
    End If

    'Ok, we can send a file.. Let them pick one
    cdlSend.FileName = vbNullString
    On Error Resume Next
    cdlSend.ShowOpen
    If Err Then Exit Sub 'They clicked cancel
    'Otherwise start the file send
    'We need to send a 'Request' message first
    lOffset = NewBuffer(oBuf)
    lMsg = MsgSendFileRequest
    AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
    AddStringToBuffer oBuf, StripFileName(cdlSend.FileName), lOffset
    dpp.SendTo lstUsers.ItemData(lstUsers.ListIndex), oBuf, 0, DPNSEND_NOLOOPBACK
    msFile = cdlSend.FileName
    mlSendToID = lstUsers.ItemData(lstUsers.ListIndex)
End Sub

Private Sub cmdWhiteBoard_Click()
    If lstUsers.ListCount < 2 Then
        MsgBox "You must have at least two people in the session before you can use the whiteboard.", vbOKOnly Or vbInformation, "Not enough people"
        Exit Sub
    End If
    If WhiteBoardWindow Is Nothing Then Set WhiteBoardWindow = New frmWhiteBoard
    WhiteBoardWindow.Show vbModeless
    'Notify everyone
    SendOpenWhiteBoardWindowMessage
    Set moCallBack = WhiteBoardWindow
End Sub

Private Sub Form_Load()
    'First start our server.  We need to be running a server in case
    'someone tries to connect to us.
    StartHosting Me
    'Add ourselves to the listbox
    lstUsers.AddItem gsUserName
    lstUsers.ItemData(0) = glMyPlayerID
    
    'Now put up our system tray icon
    With sysIcon
        .cbSize = LenB(sysIcon)
        .hwnd = Me.hwnd
        .uFlags = NIF_DOALL
        .uCallbackMessage = WM_MOUSEMOVE
        .hIcon = Me.Icon
        .sTip = "vbConferencer" & vbNullChar
    End With
    Shell_NotifyIcon NIM_ADD, sysIcon
End Sub

Private Sub Form_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Dim ShellMsg As Long
    
    ShellMsg = X / Screen.TwipsPerPixelX
    Select Case ShellMsg
    Case WM_LBUTTONDBLCLK
        ShowMyForm
    Case WM_RBUTTONUP
        'Show the menu
        'If gfStarted Then mnuStart.Enabled = False
        PopupMenu mnuPopup, , , , mnuExit
    End Select
End Sub

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    If Not mfExit Then
        Cancel = 1
        Me.Hide
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Me.Hide
    Shell_NotifyIcon NIM_DELETE, sysIcon
    Cleanup
    End
End Sub

Private Sub mnuExit_Click()
    mfExit = True
    Unload Me
End Sub

Private Sub ShowMyForm()
    Me.Visible = True
End Sub

Private Sub tmrJoin_Timer()
    tmrJoin.Enabled = False
    MsgBox "The person you are trying to reach did not accept your call.", vbOKOnly Or vbInformation, "Didn't accept"
    StartHosting Me
End Sub

Public Sub UpdatePlayerList()
    Dim lCount As Long, dpPeer As DPN_PLAYER_INFO
    Dim lInner As Long, fFound As Boolean
    Dim lTotal As Long
    
    lTotal = dpp.GetCountPlayersAndGroups(DPNENUM_PLAYERS)
    If lTotal > 1 Then cmdHangup.Enabled = True
    For lCount = 1 To lTotal
        dpPeer = dpp.GetPeerInfo(dpp.GetPlayerOrGroup(lCount))
        If (dpPeer.lPlayerFlags And DPNPLAYER_LOCAL) = DPNPLAYER_LOCAL Then
            'Don't add me
        Else
            fFound = False
            'Make sure they're not already added
            For lInner = 0 To lstUsers.ListCount - 1
                If lstUsers.ItemData(lInner) = dpp.GetPlayerOrGroup(lCount) Then fFound = True
            Next
            If Not fFound Then
                'Go ahead and add them
                lstUsers.AddItem dpPeer.Name
                lstUsers.ItemData(lstUsers.ListCount - 1) = dpp.GetPlayerOrGroup(lCount)
            End If
        End If
    Next
End Sub

Private Sub SendOpenWhiteBoardWindowMessage()
    Dim lMsg As Long, lOffset As Long
    Dim oBuf() As Byte
    
    'Now let's send a message asking the host to accept our call
    lOffset = NewBuffer(oBuf)
    lMsg = MsgShowWhiteBoard
    AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
    dpp.SendTo DPNID_ALL_PLAYERS_GROUP, oBuf, 0, DPNSEND_NOLOOPBACK
End Sub

Private Sub SendOpenChatWindowMessage()
    Dim lMsg As Long, lOffset As Long
    Dim oBuf() As Byte
    
    'Now let's send a message asking the host to accept our call
    lOffset = NewBuffer(oBuf)
    lMsg = MsgShowChat
    AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
    dpp.SendTo DPNID_ALL_PLAYERS_GROUP, oBuf, 0, DPNSEND_NOLOOPBACK
End Sub

Private Sub RemovePlayer(ByVal lPlayerID As Long)
    Dim lCount As Long
    'Remove anyone who has this player id
    For lCount = 0 To lstUsers.ListCount - 1
        If lstUsers.ItemData(lCount) = lPlayerID Then lstUsers.RemoveItem lCount
    Next
    
End Sub

Private Function StripFileName(ByVal sFile As String) As String
    'Get rid of the path to the file (Strip everything after the last \)
    If InStr(sFile, "\") Then
        StripFileName = Right$(sFile, Len(sFile) - InStrRev(sFile, "\"))
    Else
        StripFileName = sFile
    End If
End Function

Private Sub SendNextFilePart()
    
    Dim lNewMsg As Long, lNewOffSet As Long
    Dim oBuf() As Byte
    Dim lChunkSize As Long
    Static lFilePart As Long
    Static lSendCurPos As Long
    Static filSend As Long
    Dim oFile() As Byte
    
    lFilePart = lFilePart + 1
    'Send this chunk
    lNewOffSet = NewBuffer(oBuf)
    lNewMsg = MsgSendFilePart
    AddDataToBuffer oBuf, lNewMsg, LenB(lNewMsg), lNewOffSet
    'Is this chunk bigger than the amount we will send?
    If lSendCurPos + mlFileChunkSize > mlFileSize Then
        'First send the chunksize
        lChunkSize = mlFileSize - lSendCurPos
    Else
        lChunkSize = mlFileChunkSize
    End If
    AddDataToBuffer oBuf, lChunkSize, LenB(lChunkSize), lNewOffSet
    ReDim oFile(1 To lChunkSize)
    'Now read in a chunk that size
    If filSend = 0 Then
        filSend = FreeFile
        Open msFile For Binary Access Read As #filSend
    End If
    Get #filSend, , oFile
    AddDataToBuffer oBuf, oFile(1), lChunkSize, lNewOffSet
    dpp.SendTo mlSendToID, oBuf, 0, DPNSEND_NOLOOPBACK
    lSendCurPos = lSendCurPos + lChunkSize
    If lSendCurPos >= mlFileSize Then
        Close #filSend
        filSend = 0
        lSendCurPos = 0
        lFilePart = 0
        msFile = vbNullString
        mlFileSize = 0
        mlSendToID = 0
    End If
End Sub

'We will handle all of the msgs here, and report them all back to the callback sub
'in case the caller cares what's going on
Private Sub DirectPlay8Event_AddRemovePlayerGroup(ByVal lMsgID As Long, ByVal lPlayerID As Long, ByVal lGroupID As Long, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.AddRemovePlayerGroup lMsgID, lPlayerID, lGroupID, fRejectMsg
End Sub

Private Sub DirectPlay8Event_AppDesc(fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.AppDesc fRejectMsg
End Sub

Private Sub DirectPlay8Event_AsyncOpComplete(dpnotify As DxVBLibA.DPNMSG_ASYNC_OP_COMPLETE, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.AsyncOpComplete dpnotify, fRejectMsg
End Sub

Private Sub DirectPlay8Event_ConnectComplete(dpnotify As DxVBLibA.DPNMSG_CONNECT_COMPLETE, fRejectMsg As Boolean)
    Dim lMsg As Long, lOffset As Long
    Dim oBuf() As Byte
    
    If dpnotify.hResultCode = 0 Then 'Success!
        cmdHangup.Enabled = True
        'Now let's send a message asking the host to accept our call
        lOffset = NewBuffer(oBuf)
        lMsg = MsgAskToJoin
        AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
        dpp.SendTo DPNID_ALL_PLAYERS_GROUP, oBuf, 0, DPNSEND_NOLOOPBACK
    Else
        tmrUpdate.Enabled = True
    End If
    
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.ConnectComplete dpnotify, fRejectMsg
End Sub

Private Sub DirectPlay8Event_CreateGroup(ByVal lGroupID As Long, ByVal lOwnerID As Long, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.CreateGroup lGroupID, lOwnerID, fRejectMsg
End Sub

Private Sub DirectPlay8Event_CreatePlayer(ByVal lPlayerID As Long, fRejectMsg As Boolean)
    Dim dpPeer As DPN_PLAYER_INFO
    On Error Resume Next
    dpPeer = dpp.GetPeerInfo(lPlayerID)
    If (dpPeer.lPlayerFlags And DPNPLAYER_LOCAL) = DPNPLAYER_LOCAL Then
        glMyPlayerID = lPlayerID
        lstUsers.ItemData(0) = glMyPlayerID
    End If
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.CreatePlayer lPlayerID, fRejectMsg
End Sub

Private Sub DirectPlay8Event_DestroyGroup(ByVal lGroupID As Long, ByVal lReason As Long, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.DestroyGroup lGroupID, lReason, fRejectMsg
End Sub

Private Sub DirectPlay8Event_DestroyPlayer(ByVal lPlayerID As Long, ByVal lReason As Long, fRejectMsg As Boolean)
    Dim dpPeer As DPN_PLAYER_INFO
    On Error Resume Next
    If lPlayerID <> glMyPlayerID Then 'ignore removing myself
        RemovePlayer lPlayerID
    End If
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.DestroyPlayer lPlayerID, lReason, fRejectMsg
End Sub

Private Sub DirectPlay8Event_EnumHostsQuery(dpnotify As DxVBLibA.DPNMSG_ENUM_HOSTS_QUERY, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.EnumHostsQuery dpnotify, fRejectMsg
End Sub

Private Sub DirectPlay8Event_EnumHostsResponse(dpnotify As DxVBLibA.DPNMSG_ENUM_HOSTS_RESPONSE, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.EnumHostsResponse dpnotify, fRejectMsg
End Sub

Private Sub DirectPlay8Event_HostMigrate(ByVal lNewHostID As Long, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.HostMigrate lNewHostID, fRejectMsg
End Sub

Private Sub DirectPlay8Event_IndicateConnect(dpnotify As DxVBLibA.DPNMSG_INDICATE_CONNECT, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.IndicateConnect dpnotify, fRejectMsg
End Sub

Private Sub DirectPlay8Event_IndicatedConnectAborted(fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.IndicatedConnectAborted fRejectMsg
End Sub

Private Sub DirectPlay8Event_InfoNotify(ByVal lMsgID As Long, ByVal lNotifyID As Long, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.InfoNotify lMsgID, lNotifyID, fRejectMsg
End Sub

Private Sub DirectPlay8Event_Receive(dpnotify As DxVBLibA.DPNMSG_RECEIVE, fRejectMsg As Boolean)
    Dim lNewMsg As Long, lNewOffSet As Long
    Dim oBuf() As Byte
    
    Dim lMsg As Long, lOffset As Long
    Dim frmJoin As frmJoinRequest
    Dim frmTrans As frmTransferRequest
    Dim dpPeer As DPN_PLAYER_INFO
    Dim sFile As String
    
    Static lFilePart As Long
    Static lSendCurPos As Long
    Static filSend As Long
    Dim oFile() As Byte
    
    Static fil As Long, lCurPos As Long
    Dim lChunkSize As Long, oData() As Byte
    
    With dpnotify
    GetDataFromBuffer .ReceivedData, lMsg, LenB(lMsg), lOffset
    Select Case lMsg
    Case MsgChat, MsgWhisper 'Make sure chat messages get to the chat window
        If ObjPtr(moCallBack) <> ObjPtr(ChatWindow) Then
            If ChatWindow Is Nothing Then
                Set ChatWindow = New frmChat
            End If
            ChatWindow.Show
            Set moCallBack = ChatWindow
        End If
    Case MsgSendDrawPixel, MsgClearWhiteBoard
        If ObjPtr(moCallBack) <> ObjPtr(WhiteBoardWindow) Then
            If WhiteBoardWindow Is Nothing Then
                Set WhiteBoardWindow = New frmWhiteBoard
            End If
            WhiteBoardWindow.Show
            Set moCallBack = WhiteBoardWindow
        End If
    Case MsgAskToJoin
        If gfHost Then
            'We are the host, pop up the 'Ask to join dialog
            dpPeer = dpp.GetPeerInfo(dpnotify.idSender)
            Set frmJoin = New frmJoinRequest
            frmJoin.SetupRequest Me, dpnotify.idSender, dpPeer.Name
            frmJoin.Show vbModeless
        End If
    Case MsgAcceptJoin
        'We have been accepted
        'Enumerate all the players and add anyone we don't already have listed
        UpdatePlayerList
        ConnectVoice
    Case MsgRejectJoin
        'We have been rejected
        tmrJoin.Enabled = True
        'We need to use a timer here, without it, we would be attempting to cleanup
        'our dplay objects to restart our host before this message was done being processed.
    Case MsgShowChat
        'Someone wants to chat.  Open the chat window
        If ChatWindow Is Nothing Then Set ChatWindow = New frmChat
        ChatWindow.Show vbModeless
        Set moCallBack = ChatWindow
    Case MsgShowWhiteBoard
        'Someone wants to draw.  Open the whiteboard window
        If WhiteBoardWindow Is Nothing Then Set WhiteBoardWindow = New frmWhiteBoard
        WhiteBoardWindow.Show vbModeless
        Set moCallBack = WhiteBoardWindow
    Case MsgSendFileRequest
        'Someone wants to send us a file.  Should we accept?
        sFile = GetStringFromBuffer(.ReceivedData, lOffset)
        msReceiveFile = sFile
        mlReceiveSendToID = dpnotify.idSender
        dpPeer = dpp.GetPeerInfo(dpnotify.idSender)
        Set frmTrans = New frmTransferRequest
        frmTrans.SetupRequest Me, dpnotify.idSender, dpPeer.Name, sFile
        frmTrans.Show vbModeless
    Case MsgSendFileDeny
        'We don't care about this file
        msFile = vbNullString
        mlFileSize = 0
        mlSendToID = 0
    Case MsgSendFileAccept
        'Ok, they do want us to send the file to them.. We will send it in chunks
        'First we will send the file info
        lNewOffSet = NewBuffer(oBuf)
        lMsg = MsgSendFileInfo
        AddDataToBuffer oBuf, lMsg, LenB(lMsg), lNewOffSet
        mlFileSize = FileLen(msFile)
        AddDataToBuffer oBuf, mlFileSize, LenB(mlFileSize), lNewOffSet
        dpp.SendTo mlSendToID, oBuf, 0, DPNSEND_NOLOOPBACK Or DPNSEND_GUARANTEED
        SendNextFilePart
    Case MsgSendFileInfo
        'They just send us the file size, save it
        GetDataFromBuffer .ReceivedData, mlReceiveFileSize, LenB(mlReceiveFileSize), lOffset
        frmProgress.Show
        frmProgress.SetFile msReceiveFile
        frmProgress.SetMax mlReceiveFileSize
        frmProgress.SetValue 0
    Case MsgSendFilePart
        GetDataFromBuffer .ReceivedData, lChunkSize, LenB(lChunkSize), lOffset
        ReDim oData(1 To lChunkSize)
        'We just received a file part..  Append this to our current file
        If fil = 0 Then
            fil = FreeFile
            If Dir$(App.Path & "\" & msReceiveFile) <> vbNullString Then Kill App.Path & "\" & msReceiveFile
            Open App.Path & "\" & msReceiveFile For Binary Access Write As #fil
        End If
        GetDataFromBuffer .ReceivedData, oData(1), lChunkSize, lOffset
        Put #fil, , oData
        'Is this the end of the file?
        lCurPos = lCurPos + lChunkSize
        frmProgress.SetValue lCurPos
        If lCurPos >= mlReceiveFileSize Then
            'We're done with the file
            Close #fil
            MsgBox "Successfully received " & msReceiveFile & ".", vbOKOnly Or vbInformation, "Complete"
            Unload frmProgress
            mlReceiveFileSize = 0
            msReceiveFile = vbNullString
            fil = 0
            lCurPos = 0
        Else
            'Acknowledge that we received this part
            lNewMsg = MsgAckFilePart
            lNewOffSet = NewBuffer(oBuf)
            AddDataToBuffer oBuf, lNewMsg, LenB(lNewMsg), lNewOffSet
            dpp.SendTo dpnotify.idSender, oBuf, 0, DPNSEND_NOLOOPBACK Or DPNSEND_GUARANTEED
        End If
    Case MsgAckFilePart
        SendNextFilePart
    End Select
    End With
    
    If (Not moCallBack Is Nothing) Then moCallBack.Receive dpnotify, fRejectMsg
End Sub

Private Sub DirectPlay8Event_SendComplete(dpnotify As DxVBLibA.DPNMSG_SEND_COMPLETE, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.SendComplete dpnotify, fRejectMsg
End Sub

Private Sub DirectPlay8Event_TerminateSession(dpnotify As DxVBLibA.DPNMSG_TERMINATE_SESSION, fRejectMsg As Boolean)
    'VB requires that we must implement *every* member of this interface
    If (Not moCallBack Is Nothing) Then moCallBack.TerminateSession dpnotify, fRejectMsg
End Sub

Private Sub tmrUpdate_Timer()
    tmrUpdate.Enabled = False
    MsgBox "The person you are trying to reach is not available.", vbOKOnly Or vbInformation, "Unavailable"
    StartHosting Me
End Sub
