program broadcast;

{$APPTYPE CONSOLE}

uses
  Windows, SysUtils, HawkNL;

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

procedure mainTestLoop(sock: NLsocket);
var
  buffer: array [0..99] of Char;
  str: array [0..NL_MAX_STRING_LENGTH - 1] of Char;
  addr: NLaddress;
  hello: PChar;
begin
  hello := 'Hello';
  while true do
  begin
    nlWrite(sock, hello^, StrLen(hello) + 1);

    while nlRead(sock, buffer, SizeOf(buffer)) > 0 do
    begin
      nlGetRemoteAddr(sock, addr);
      buffer[99] := #0;
      Writeln(Format('received %s from %s, packet #%d', [buffer, nlAddrToString(addr, str), nlGetInteger(NL_PACKETS_RECEIVED)]));
    end;
    Sleep(1000);
  end;
end;

var
  sock: NLsocket;
  addr: NLaddress;
  alladdr: PNLaddress;
  str: array [0..NL_MAX_STRING_LENGTH - 1] of Char;
  networkType: NLenum; (* default network type *)
  count: NLint;
begin
  networkType := NL_IP;

  if not nlInit() then
    printErrorExit();

  Writeln('nlGetString(NL_VERSION) = ', nlGetString(NL_VERSION));
  Writeln;
  Writeln('nlGetString(NL_NETWORK_TYPES) = ', nlGetString(NL_NETWORK_TYPES));
  WriteLn;

  if ParamCount >= 1 then
  begin
    if ParamStr(1) = 'NL_IPX' then
      networkType := NL_IPX
    else if ParamStr(1) = 'NL_LOOP_BACK' then
      networkType := NL_LOOP_BACK;
  end;

  if not nlSelectNetwork(networkType) then
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
  nlEnable(NL_SOCKET_STATS);
  (* enable reuse address to run two or more copies on one machine *)
  nlHint(NL_REUSE_ADDRESS, NL_TRUE);

  (* create a client socket *)
  sock := nlOpen(25000, NL_BROADCAST);

  if sock = NL_INVALID then
    printErrorExit();

  nlGetLocalAddr(sock, addr);
  Writeln('socket address is ', nlAddrToString(addr, str));
  mainTestLoop(sock);

  nlShutdown();
end.
