(*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2001-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*)
(*
  This app opens up UDP/TCP sockets until the system cannot open up any more,
  or it hits the NL_MAX_INT_SOCKETS limit. On a Windows NT 4.0 server with
  256 MB of RAM, it can open up 64511 UDP sockets and 7926 connected
  TCP sockets, and on Windows 95 it can open up 252 UDP sockets.
*)

program max;

{$APPTYPE CONSOLE}

uses
  SysUtils,
  HawkNL;

procedure printErrorExit;
var
  err: NLenum;
begin
  err := nlGetError();

  if err = NL_SYSTEM_ERROR then
    Writeln('System error: ', nlGetSystemErrorStr(nlGetSystemError()))
  else
    Writeln('HawkNL error: ', nlGetErrorStr(err));
  nlShutdown();
  Halt(1);
end;

var
  sock, serversock, s: NLsocket;
  address, serveraddr: NLaddress;
  alladdr: PNLaddress;
  group, count: NLint;
  str: array [0..NL_MAX_STRING_LENGTH - 1] of Char;
begin
  if not nlInit() then
    printErrorExit();

  Writeln('nlGetString(NL_VERSION) = ', nlGetString(NL_VERSION));
  Writeln('nlGetString(NL_NETWORK_TYPES) = ', nlGetString(NL_NETWORK_TYPES));

  if not nlSelectNetwork(NL_IP) then
    printErrorExit();

  (* list all the local addresses *)
  Writeln('local addresses are:');
  alladdr := nlGetAllLocalAddr(count);
  while count > 0 do
  begin
    Writeln('  ', nlAddrToString(alladdr^, str));
    alladdr := PNLaddress(Integer(alladdr) + SizeOf(NLaddress));
    count := count - 1;
  end;
  Writeln;
  Writeln('Testing UDP sockets');
  Writeln;
  while true do
  begin
    (* create a client socket *)
    sock := nlOpen(0, NL_UNRELIABLE); (* let the system assign the port number *)
    if sock = NL_INVALID then
      Break;
    nlGetLocalAddr(sock, address);
    Write(Format('Socket: %d, port: %d'#13, [sock, nlGetPortFromAddr(address)]));
  end;

  Writeln;
  Writeln(Format('Opened %d sockets', [nlGetInteger(NL_OPEN_SOCKETS)]));
  nlShutdown();

  if not nlInit() then
    printErrorExit();
  if not nlSelectNetwork(NL_IP) then
    printErrorExit();

  Writeln('Testing TCP connected sockets');
  Writeln;
  serversock := nlOpen(1258, NL_RELIABLE);
  nlListen(serversock);
  nlGetLocalAddr(serversock, serveraddr);
  nlSetAddrPort(serveraddr, 1258);
  group := nlGroupCreate();
  nlGroupAddSocket(group, serversock);
  while true do
  begin
    (* create a client socket *)
    sock := nlOpen(0, NL_RELIABLE); (* let the system assign the port number *)
    if sock = NL_INVALID then
      Break;
    nlConnect(sock, serveraddr);
    Write('Socket1: ', sock, #13);
    if nlPollGroup(group, NL_READ_STATUS, s, 1, 1000) <> 1 then
      Break;
    sock := nlAcceptConnection(serversock);
    if sock = NL_INVALID then
      Break;
    Write('Socket2: ', sock, #13);
  end;
  Writeln;
  Writeln(Format('Opened %d sockets', [nlGetInteger(NL_OPEN_SOCKETS)]));
  nlShutdown();
end.
