object MainForm: TMainForm
  Left = 564
  Top = 184
  Width = 417
  Height = 251
  Caption = 'Synchronet Chat'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -14
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poDefaultPosOnly
  OnClose = FormClose
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 120
  TextHeight = 16
  object Splitter1: TSplitter
    Left = 0
    Top = 105
    Width = 409
    Height = 2
    Cursor = crVSplit
    Align = alTop
  end
  object Local: TMemo
    Left = 0
    Top = 107
    Width = 409
    Height = 112
    Align = alClient
    ReadOnly = True
    ScrollBars = ssVertical
    TabOrder = 0
    OnKeyPress = LocalKeyPress
  end
  object Remote: TMemo
    Left = 0
    Top = 0
    Width = 409
    Height = 105
    TabStop = False
    Align = alTop
    HideSelection = False
    ReadOnly = True
    ScrollBars = ssVertical
    TabOrder = 1
  end
  object InputTimer: TTimer
    OnTimer = InputTimerTick
    Left = 72
    Top = 40
  end
  object Timer: TTimer
    Enabled = False
    OnTimer = TimerTick
    Left = 128
    Top = 48
  end
end
