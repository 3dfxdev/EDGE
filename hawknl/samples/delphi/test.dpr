program test;

{$APPTYPE CONSOLE}

uses
  Windows,
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

const
  MAX_CLIENTS = 10;

procedure mainServerLoop(sock: NLsocket);
var
  client: array [0..MAX_CLIENTS - 1] of NLsocket;
  clientnum: NLint;
  str: array [0..NL_MAX_STRING_LENGTH - 1] of Char;
  newsock: NLsocket;
  addr: NLaddress;
  i, j: NLint;
  buffer: array [0..127] of Char;
begin
  clientnum := 0;
  FillChar(client, SizeOf(client), 0);

  while true do
  begin
    (* check for a new client *)
    newsock := nlAcceptConnection(sock);

    if newsock <> NL_INVALID then
    begin
      nlGetRemoteAddr(newsock, addr);
      client[clientnum] := newsock;
      Writeln(Format('Client %d connected from %s', [clientnum, nlAddrToString(addr, str)]));
      clientnum := clientnum + 1;
    end
    else
    begin
      if nlGetError() = NL_SYSTEM_ERROR then
        printErrorExit();
    end;
    (* loop through the clients and read the packets *)
    for i := 0 to clientnum - 1 do
    begin
      if nlRead(client[i], buffer, SizeOf(buffer)) > 0 then
      begin
        buffer[127] := #0; (* null terminate the char string *)
        Writeln(Format('Client %d sent %s', [i, buffer]));
        for j := 0 to clientnum - 1 do
        begin
          if i <> j then
            nlWrite(client[j], buffer, StrLen(buffer));
        end;
      end;
    end;
    Sleep(0);
  end;
end;

procedure mainClientLoop(sock: NLsocket);
var
  buffer: array [0..127] of Char;
begin
  while true do
  begin
    FillChar(buffer, SizeOf(buffer), 0);
    Readln(buffer);
    nlWrite(sock, buffer, StrLen(buffer) + 1);
    while nlRead(sock, buffer, SizeOf(buffer)) > 0 do
      Writeln(buffer);
    Sleep(0);
  end;
end;

var
  isserver: NLboolean;
  serversock: NLsocket;
  clientsock: NLsocket;
  addr: NLaddress;
  server: PChar;
  networkType: NLenum;
  str: array [0..NL_MAX_STRING_LENGTH - 1] of Char;
begin
  isserver := NL_FALSE;
  server := '127.0.0.1:25000';
  networkType := NL_UNRELIABLE; (* Change this to NL_RELIABLE for reliable connection *)

  if not nlInit() then
    printErrorExit();

  Writeln('nlGetString(NL_VERSION) = ', nlGetString(NL_VERSION));
  Writeln;
  Writeln('nlGetString(NL_NETWORK_TYPES) = ', nlGetString(NL_NETWORK_TYPES));
  Writeln;

  if not nlSelectNetwork(NL_IP) then
    printErrorExit();

  if ParamCount >= 1 then
  begin
    if ParamStr(1) = '-s' then (* server mode *)
      isserver := NL_TRUE;
  end;

  if isserver then
  begin
    (* create a server socket *)
    serversock := nlOpen(25000, networkType); (* just a random port number ;) *)

    if serversock = NL_INVALID then
      printErrorExit();

    if not nlListen(serversock) then       (* let's listen on this socket *)
    begin
      nlClose(serversock);
      printErrorExit();
    end;
    nlGetLocalAddr(serversock, addr);
    Writeln('Server address is ', nlAddrToString(addr, str));
    mainServerLoop(serversock);
  end
  else
  begin
    (* create a client socket *)
    clientsock := nlOpen(0, networkType); (* let the system assign the port number *)
    nlGetLocalAddr(clientsock, addr);
    Writeln('our address is ', nlAddrToString(addr, str));

    if clientsock = NL_INVALID then
      printErrorExit();
    (* create the NLaddress *)
    nlStringToAddr(server, addr);
    Writeln('Address is ', nlAddrToString(addr, str));
    (* now connect *)
    if not nlConnect(clientsock, addr) then
    begin
      nlClose(clientsock);
      printErrorExit();
    end;
    mainClientLoop(clientsock);
  end;

  nlShutdown();
end.

