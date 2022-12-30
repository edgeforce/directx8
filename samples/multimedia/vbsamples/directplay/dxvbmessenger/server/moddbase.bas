Attribute VB_Name = "modDBase"
Option Explicit
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'
'  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
'
'  File:       modDBase.bas
'
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

Private Type FriendOnlineType
    sFriendName As String
    fOnline As Boolean
    fFriend As Boolean
End Type

Public Enum LogonTypes
    LogonSuccess
    InvalidPassword
    AccountDoesNotExist
End Enum

Private Const msDoubleQuote As String = """"
Public goConn As Connection

Public Sub OpenClientDatabase()

    On Error GoTo ErrOut
    Dim sMedia As String
    
    sMedia = FindMediaDir("vbMsgSrv.mdb")
    'Create a new ADO Connection
    Set goConn = New Connection
    'Open the database
    goConn.Open "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" & sMedia & "vbMsgSrv.mdb;Mode=Read|Write"
    Exit Sub
ErrOut:
    MsgBox "Could not connect to the server database.  Exiting", vbOKOnly Or vbInformation, "Exiting."
    End
End Sub

Public Sub CloseDownDB()
    goConn.Close
End Sub

'Check to see if this user already exists.  If they do, then we can't create a new account
'with this username.
Public Function DoesUserExist(ByVal sUsername As String) As Boolean

    Dim myRs As New Recordset
    
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sUsername & msDoubleQuote, goConn
    If Not myRs.EOF Then 'This user did exist
        DoesUserExist = True
        Exit Function
    End If
    DoesUserExist = False
    
End Function

Public Function LogonUser(ByVal sUsername As String, sPwd As String) As LogonTypes

    Dim myRs As New Recordset
    Dim sPassword As String
    
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sUsername & msDoubleQuote, goConn
    If myRs.EOF Then 'This user did not exist
        LogonUser = AccountDoesNotExist
        Exit Function
    End If
    'Ok, this user does exist.  First lets decrypt the password sent from the client
    sPassword = EncodePassword(sPwd, glClientSideEncryptionKey)
    'Now check this password against what's listed in the db.
    If myRs.Fields("ClientPassword").Value = EncodePassword(sPassword, glServerSideEncryptionKey) Then
        'The passwords match, logon was successful
        LogonUser = LogonSuccess
        Exit Function
    Else
        'Invalid password, let the user know
        LogonUser = InvalidPassword
        Exit Function
    End If
End Function

Public Sub AddUser(ByVal sUsername As String, sPwd As String, ByVal lCurrentDPlayID As Long)
    Dim sPassword As String
    Dim sSql As String
    
    'First decrypt the password
    sPassword = EncodePassword(sPwd, glClientSideEncryptionKey)
    'Add this user, and set the flag to show that we are currently logged on, and keep our current DPlay ID
    sSql = "Insert Into ClientInfo (ClientName, ClientPassword, CurrentlyLoggedOn, CurrentDPlayID) Values "
    sSql = sSql & " (" & msDoubleQuote & sUsername & msDoubleQuote & ", " & msDoubleQuote & EncodePassword(sPassword, glServerSideEncryptionKey) & msDoubleQuote
    sSql = sSql & ", 1, " & CStr(lCurrentDPlayID) & ")"
    'Now perform the action
    goConn.Execute sSql
End Sub

Public Sub UpdateDBToShowLogon(sPlayer As String, ByVal lCurrentDPlayID As Long)
    'Set the flag to show that we are currently logged on, and keep our current DPlay ID
    goConn.Execute "Update ClientInfo Set CurrentlyLoggedOn=1, CurrentDplayID=" & CStr(lCurrentDPlayID) & " where ClientName = " & msDoubleQuote & sPlayer & msDoubleQuote
End Sub

Public Sub UpdateDBToShowLogoff(ByVal lCurrentDPlayID As Long)
    'Set the flag to show that we are currently logged on, and keep our current DPlay ID
    goConn.Execute "Update ClientInfo Set CurrentlyLoggedOn=0, CurrentDplayID=0 where CurrentDplayID = " & CStr(lCurrentDPlayID)
End Sub

Public Sub NotifyFriendsImOnline(sPlayer As String)
    Dim myRs As New Recordset, lMsg As Long
    Dim oBuf() As Byte, lOffset As Long
    Dim sMyClientID As String
    Dim myTempRS As New Recordset
    
    'First we need to find out if I'm on anyones friends list
    'Get my ClientID first
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sPlayer & msDoubleQuote, goConn
    'We need to get the clientid of this person
    sMyClientID = myRs.Fields("ClientID").Value
    myRs.Close
    'Now see if I'm anyone's friends
    myRs.Open "Select * from FriendList where FriendID = " & sMyClientID, goConn
    Do While Not myRs.EOF 'Yup, I am.  Notify each of them that I just logged on
        'First check to see if they are logged on
        myTempRS.Open "Select * from ClientInfo where ClientID = " & CStr(myRs.Fields("ClientID").Value), goConn
        If myTempRS.Fields("CurrentDPlayID").Value <> 0 Then 'Notify this person I'm online
            lMsg = Msg_FriendLogon
            lOffset = NewBuffer(oBuf)
            AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
            AddStringToBuffer oBuf, sPlayer, lOffset
            dps.SendTo CLng(myTempRS.Fields("CurrentDPlayID").Value), oBuf, 0, 0
        End If
        myTempRS.Close
        myRs.MoveNext
    Loop
    
End Sub

Public Sub NotifyFriendsImOffline(sPlayer As String)
    Dim myRs As New Recordset, lMsg As Long
    Dim oBuf() As Byte, lOffset As Long
    Dim sMyClientID As String
    Dim myTempRS As New Recordset
    
    'First we need to find out if I'm on anyones friends list
    'Get my ClientID first
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sPlayer & msDoubleQuote, goConn
    'We need to get the clientid of this person
    sMyClientID = myRs.Fields("ClientID").Value
    myRs.Close
    'Now see if I'm anyone's friends
    myRs.Open "Select * from FriendList where FriendID = " & sMyClientID, goConn
    Do While Not myRs.EOF 'Yup, I am.  Notify each of them that I just logged on
        'First check to see if they are logged on
        myTempRS.Open "Select * from ClientInfo where ClientID = " & CStr(myRs.Fields("ClientID").Value), goConn
        If myTempRS.Fields("CurrentDPlayID").Value <> 0 Then 'Notify this person I'm online
            lMsg = Msg_FriendLogoff
            lOffset = NewBuffer(oBuf)
            AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
            AddStringToBuffer oBuf, sPlayer, lOffset
            dps.SendTo CLng(myTempRS.Fields("CurrentDPlayID").Value), oBuf, 0, 0
        End If
        myTempRS.Close
        myRs.MoveNext
    Loop
    
End Sub

Public Sub GetFriendsOfMineOnline(sPlayer As String)
    Dim myRs As New Recordset, lMsg As Long
    Dim oBuf() As Byte, lOffset As Long
    Dim sMyClientID As String
    Dim myTempRS As New Recordset
    
    Dim lSendID As Long
    Dim oFriends() As FriendOnlineType
    Dim lCount As Long
    
    'First we need to find out if I'm on anyones friends list
    'Get my ClientID first
    LogInfo "(GetFriends) SQL =Select * from ClientInfo where ClientName = " & msDoubleQuote & sPlayer & msDoubleQuote
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sPlayer & msDoubleQuote, goConn
    'We need to get the clientid of this person
    sMyClientID = myRs.Fields("ClientID").Value
    lSendID = CLng(myRs.Fields("CurrentDPlayID").Value)
    myRs.Close
    'Now see if I have any friends
    LogInfo "(GetFriends) Friends SQL =Select * from FriendList where ClientID = " & sMyClientID
    myRs.Open "Select * from FriendList where ClientID = " & sMyClientID, goConn
    lMsg = Msg_SendClientFriends
    lOffset = NewBuffer(oBuf)
    AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
    ReDim oFriends(0)
    Do While Not myRs.EOF 'Yup, I do.  Send me the list first
        'First check to see if they are logged on
        ReDim Preserve oFriends(UBound(oFriends) + 1)
        myTempRS.Open "Select * from ClientInfo where ClientID = " & CStr(myRs.Fields("FriendID").Value), goConn
        With oFriends(UBound(oFriends))
            .sFriendName = myTempRS.Fields("ClientName").Value
            .fOnline = (myTempRS.Fields("CurrentDPlayID").Value <> 0)
            .fFriend = myRs.Fields("Friend").Value
        End With
        myTempRS.Close
        myRs.MoveNext
    Loop
    myRs.Close
    AddDataToBuffer oBuf, CLng(UBound(oFriends)), SIZE_LONG, lOffset
    For lCount = 1 To UBound(oFriends)
        AddDataToBuffer oBuf, oFriends(lCount).fFriend, LenB(oFriends(lCount).fFriend), lOffset
        AddStringToBuffer oBuf, oFriends(lCount).sFriendName, lOffset
    Next
    dps.SendTo lSendID, oBuf, 0, 0
    For lCount = 1 To UBound(oFriends)
        If oFriends(lCount).fOnline Then
            ReDim oBuf(0)
            lMsg = Msg_FriendLogon
            lOffset = NewBuffer(oBuf)
            AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
            AddStringToBuffer oBuf, oFriends(lCount).sFriendName, lOffset
            dps.SendTo lSendID, oBuf, 0, 0
        End If
    Next
    
End Sub

'If fFriend is True, then this person is a friend.  If it is False, then the person is blocked
Public Function AddFriend(ByVal lPlayerID As Long, sFriendName As String, ByVal fFriend As Boolean) As Boolean

    Dim myRs As New Recordset
    Dim sFriendClient As String, sMyClientID As String
    
    AddFriend = False
    LogInfo "(AddFriend) In Function: SQL= Select * from ClientInfo where ClientName = " & msDoubleQuote & sFriendName & msDoubleQuote
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sFriendName & msDoubleQuote, goConn
    'We need to get the clientid of this friend
    sFriendClient = myRs.Fields("ClientID").Value
    If myRs.Fields("CurrentDPlayID").Value <> 0 Then AddFriend = True
    myRs.Close
    LogInfo "(AddFriend) Got ClientID of Friend:"
    
    LogInfo "(AddFriend) Get ClientID of me:"
    myRs.Open "Select * from ClientInfo where CurrentDPlayID = " & CStr(lPlayerID), goConn
    'We need to get the clientid of this person
    sMyClientID = myRs.Fields("ClientID").Value
    myRs.Close
    LogInfo "(AddFriend) Got ClientID of me:"
    
    'First we need to check if this user is already a friend or blocked, and if so, just update them
    LogInfo "(AddFriend) Is this a friend already?"
    myRs.Open "Select count(ClientID) from FriendList where ((ClientID = " & sMyClientID & ") and (FriendID = " & sFriendClient & "))"
    If myRs.Fields(0).Value = 0 Then 'No one, add this one
        'Now add this friend to the list
        LogInfo "(AddFriend) No, add:"
        goConn.Execute "Insert into FriendList values (" & sMyClientID & "," & sFriendClient & ", " & CStr(Abs(fFriend)) & ")"
    Else
        'Update the record that already exists
        LogInfo "(AddFriend) Yes, update: SQL = Update FriendList Set Friend = " & CStr(Abs(fFriend)) & " where ((ClientID = " & sMyClientID & ") and (FriendID = " & sFriendClient & "))"
        goConn.Execute "Update FriendList Set Friend = " & CStr(Abs(fFriend)) & " where ((ClientID = " & sMyClientID & ") and (FriendID = " & sFriendClient & "))"
    End If
End Function

Public Function GetCurrentDPlayID(sPlayer As String) As Long
    Dim myRs As New Recordset
    
    'Get my ClientID first
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sPlayer & msDoubleQuote, goConn
    'We need to get the current dplay id of this person
    GetCurrentDPlayID = CLng(myRs.Fields("CurrentDPlayID").Value)
    myRs.Close
End Function

Public Function AmIBlocked(sMe As String, sFriend As String) As Boolean
    Dim myRs As New Recordset
    Dim sMyClientID As String
    Dim sFriendClientID As String
    Dim myTempRS As New Recordset
    
    'Get my ClientID first
    LogInfo "(AmIBlocked) SQL =Select * from ClientInfo where ClientName = " & msDoubleQuote & sMe & msDoubleQuote
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sMe & msDoubleQuote, goConn
    'We need to get the clientid of this person
    sMyClientID = myRs.Fields("ClientID").Value
    myRs.Close
    'Now get my FriendsID
    LogInfo "(AmIBlocked) SQL =Select * from ClientInfo where ClientName = " & msDoubleQuote & sFriend & msDoubleQuote
    myRs.Open "Select * from ClientInfo where ClientName = " & msDoubleQuote & sFriend & msDoubleQuote, goConn
    'We need to get the clientid of this person
    sFriendClientID = myRs.Fields("ClientID").Value
    myRs.Close
    
    LogInfo "(AmIBlocked) SQL = Select * from FriendList where ClientID = " & sMyClientID & " and FriendID = " & sFriendClientID
    myRs.Open "Select * from FriendList where ClientID = " & sMyClientID & " and FriendID = " & sFriendClientID, goConn
    On Error Resume Next
    AmIBlocked = Not (myRs.Fields("Friend").Value)
    myRs.Close
    LogInfo "(AmIBlocked) Leaving: Retval = " & CStr(AmIBlocked)
End Function
