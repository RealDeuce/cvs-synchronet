object ServicesForm: TServicesForm
  Left = 387
  Top = 289
  Width = 737
  Height = 208
  Caption = 'Services'
  Color = clBtnFace
  DragKind = dkDock
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnHide = FormHide
  PixelsPerInch = 120
  TextHeight = 16
  object ToolBar: TToolBar
    Left = 0
    Top = 0
    Width = 729
    Height = 25
    Caption = 'ToolBar'
    EdgeBorders = []
    Flat = True
    Images = MainForm.ImageList
    ParentShowHint = False
    ShowHint = True
    TabOrder = 0
    object StartButton: TToolButton
      Left = 0
      Top = 0
      Action = MainForm.ServicesStart
      Grouped = True
      ParentShowHint = False
      ShowHint = True
    end
    object LogPauseButton: TToolButton
      Left = 23
      Top = 0
      Hint = 'Pause Log Scrolling'
      Caption = 'LogPauseButton'
      Enabled = False
      ImageIndex = 67
    end
    object StopButton: TToolButton
      Left = 46
      Top = 0
      Action = MainForm.ServicesStop
      Grouped = True
      ParentShowHint = False
      ShowHint = True
    end
    object RecycleButton: TToolButton
      Left = 69
      Top = 0
      Action = MainForm.ServicesRecycle
    end
    object ToolButton1: TToolButton
      Left = 92
      Top = 0
      Width = 8
      Caption = 'ToolButton1'
      ImageIndex = 1
      Style = tbsSeparator
    end
    object ConfigureButton: TToolButton
      Left = 100
      Top = 0
      Action = MainForm.ServicesConfigure
      ParentShowHint = False
      ShowHint = True
    end
    object ToolButton2: TToolButton
      Left = 123
      Top = 0
      Width = 8
      Caption = 'ToolButton2'
      ImageIndex = 5
      Style = tbsSeparator
    end
    object Status: TStaticText
      Left = 131
      Top = 0
      Width = 150
      Height = 22
      Hint = 'Services Status'
      Align = alClient
      AutoSize = False
      BorderStyle = sbsSunken
      Caption = 'Down'
      TabOrder = 0
    end
    object ToolButton3: TToolButton
      Left = 281
      Top = 0
      Width = 8
      Caption = 'ToolButton3'
      ImageIndex = 6
      Style = tbsSeparator
    end
    object LogLevelText: TStaticText
      Left = 289
      Top = 0
      Width = 75
      Height = 22
      Hint = 'Log Level'
      AutoSize = False
      BorderStyle = sbsSunken
      TabOrder = 1
    end
    object LogLevelUpDown: TUpDown
      Left = 364
      Top = 0
      Width = 16
      Height = 22
      Hint = 'Log Level Adjustment'
      Min = 0
      Max = 7
      Position = 0
      TabOrder = 2
      Wrap = False
      OnChangingEx = LogLevelUpDownChangingEx
    end
  end
  object Log: TRichEdit
    Left = 0
    Top = 25
    Width = 729
    Height = 150
    Align = alClient
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -14
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    HideScrollBars = False
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssBoth
    TabOrder = 1
    WordWrap = False
  end
end
