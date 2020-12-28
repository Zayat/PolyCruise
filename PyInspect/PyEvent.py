#!/usr/bin/python

import sys
import abc
from ast import Name

class LiveObject ():
    def __init__(self):
        self.Def = None
        self.Uses = []
        self.Callee = None
        self.Ret = False
        self.LineNo = 0

    def SetDef (self, Def):
        self.Def = Def

    def SetLineNo (self, LineNo):
        self.LineNo = LineNo

    def SetUse (self, Use):
        self.Uses.append (Use)

    def SetCallee (self, CallFunc):
        self.Callee = CallFunc

    def SetRet (self, RetFlg = False):
        self.Ret = RetFlg

    def View (self):
        print ("==>[",  self.LineNo, "]Def: ", self.Def, " Use: ", self.Uses, " Call: ", self.Callee, " Ret: ", self.Ret)

class PyEvent(metaclass=abc.ABCMeta):
    def __init__(self, Frame, Event, Statement):
        self.Frame = Frame
        self.Event = Event
        self.Statement = Statement
        self.LiveObj = LiveObject ()
  
    @abc.abstractmethod
    def GetDefUse (self):
        pass

    def GetLiveObject(self, ValName):
        if isinstance(ValName, Name):
            ValName = ValName.id
        
        Frame = self.Frame
        if ValName in Frame.f_locals:
            return Frame.f_locals[ValName]
        elif ValName in Frame.f_globals:
            return Frame.f_globals[ValName]
        else:
            Builtins = Frame.f_globals['__builtins__']
            if isinstance(Builtins, dict) and (ValName in Builtins):
                return Builtins[ValName]
            elif isinstance(Builtins, ModuleType) and hasattr(Builtins, ValName):
                return getattr(Frame.f_globals['__builtins__'], ValName)
        raise ValueError('cannot find the value of {name}'
                         ' in frame of {frame}'.format(name=ValName, frame=Frame.f_code.co_name))

    def Default(self):
        raise NotImplementedError(self.errorinfo('not implement this kind of '
                                                 'method: {}'.format(dump(self.Statement))))
        
   
