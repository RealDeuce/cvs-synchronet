object CtrlPathDialog: TCtrlPathDialog
  Left = 352
  Top = 625
  ActiveControl = Edit
  BorderStyle = bsDialog
  Caption = 'Synchronet Configuration File'
  ClientHeight = 93
  ClientWidth = 384
  Color = clBtnFace
  ParentFont = True
  OldCreateOrder = True
  Position = poScreenCenter
  PixelsPerInch = 96
  TextHeight = 13
  object Bevel1: TBevel
    Left = 8
    Top = 8
    Width = 281
    Height = 76
    Anchors = [akLeft, akTop, akBottom]
    Shape = bsFrame
  end
  object OKBtn: TButton
    Left = 300
    Top = 8
    Width = 75
    Height = 25
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 0
  end
  object CancelBtn: TButton
    Left = 300
    Top = 38
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
  end
  object Edit: TEdit
    Left = 20
    Top = 20
    Width = 260
    Height = 21
    TabOrder = 2
    Text = 'c:\sbbs\ctrl\main.cnf'
  end
  object BrowseButton: TButton
    Left = 20
    Top = 51
    Width = 76
    Height = 25
    Caption = 'Browse...'
    TabOrder = 3
    OnClick = BrowseButtonClick
  end
  object OpenDialog: TOpenDialog
    FileName = 'main.cnf'
    Filter = 'Synchronet Configuration File|main.cnf'
    Left = 424
    Top = 88
  end
end
