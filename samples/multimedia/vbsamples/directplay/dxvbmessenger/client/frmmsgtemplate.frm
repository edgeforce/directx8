VERSION 5.00
Begin VB.Form frmMsgTemplate 
   Caption         =   "Message"
   ClientHeight    =   4665
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4680
   Icon            =   "frmMsgTemplate.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   4665
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtSendData 
      Height          =   615
      Left            =   0
      MultiLine       =   -1  'True
      TabIndex        =   0
      Top             =   4020
      Width           =   4635
   End
   Begin VB.TextBox txtConversation 
      Height          =   3915
      Left            =   0
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   1
      Top             =   0
      Width           =   4635
   End
End
Attribute VB_Name = "frmMsgTemplate"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'
'  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
'
'  File:       frmMsgTemplate.frm
'
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

Private msUser As String

'The username property lets us make sure messages get routed to the right place
Public Property Let UserName(ByVal sUser As String)
    msUser = sUser
    Me.Caption = "Message - " & sUser
End Property

Public Property Get UserName() As String
    UserName = msUser
End Property

Public Sub AddChatMessage(ByVal sChat As String, Optional ByVal fMeTalking As Boolean = False)
    
    If fMeTalking Then
        sChat = "<" & gsUserName & "> " & sChat
    Else
        sChat = "<" & msUser & "> " & sChat
    End If
    'Update the chat window first
    txtConversation.Text = txtConversation.Text & sChat & vbCrLf
    'Now limit the text in the window to be 32k
    If Len(txtConversation.Text) > 32767 Then
        txtConversation.Text = Right$(txtConversation.Text, 32767)
    End If
    'Autoscroll the text
    txtConversation.SelStart = Len(txtConversation.Text)

End Sub

Private Sub Form_GotFocus()
    On Error Resume Next
    txtSendData.SetFocus
End Sub

Private Sub Form_Load()
    Me.Caption = "Message - " & msUser
End Sub

Private Sub Form_Resize()
    If Me.WindowState <> vbMinimized Then
        txtConversation.Move Screen.TwipsPerPixelX, Screen.TwipsPerPixelY, Me.Width - (3 * Screen.TwipsPerPixelX), Me.Height - (txtSendData.Height + (5 * Screen.TwipsPerPixelY))
        txtSendData.Move Screen.TwipsPerPixelX, Me.Height - (txtSendData.Height + (2 * Screen.TwipsPerPixelY)), Me.Width - (3 * Screen.TwipsPerPixelX)
    End If
End Sub

Private Sub txtSendData_KeyPress(KeyAscii As Integer)
    Dim lMsg As Long
    Dim oBuf() As Byte, lOffset As Long

    If KeyAscii = vbKeyReturn Then 'Send this message
        If txtSendData.Text <> vbNullString Then
            lMsg = Msg_SendMessage
            lOffset = NewBuffer(oBuf)
            AddDataToBuffer oBuf, lMsg, LenB(lMsg), lOffset
            AddStringToBuffer oBuf, msUser, lOffset
            AddStringToBuffer oBuf, gsUserName, lOffset
            AddStringToBuffer oBuf, txtSendData.Text, lOffset
            dpc.Send oBuf, 0, 0
            AddChatMessage txtSendData.Text, True
        End If
        KeyAscii = 0
        txtSendData.Text = vbNullString
    End If
    
End Sub
