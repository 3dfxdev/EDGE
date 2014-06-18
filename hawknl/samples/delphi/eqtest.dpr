program eqtest;

{$APPTYPE CONSOLE}

uses
  HawkNL;

const
{$J+}
  command: array [0..3] of NLubyte = ($FF, $FF, $09, $00);
{$J-}
  port = 24252;
  server = 'status.everquest.com';
var
  sock: NLsocket;
  addr: NLaddress;
  buffer: array [0..1023] of Char;
  count: NLint;
begin
  if not nlInit() then
    Halt(1);
  if not nlSelectNetwork(NL_IP) then
  begin
    nlShutdown();
    Halt(1);
  end;
  nlEnable(NL_BLOCKING_IO);
  (* create server the address *)
  nlGetAddrFromName(server, addr);
  nlSetAddrPort(addr, port);
  (* create the socket *)
  sock := nlOpen(0, NL_UNRELIABLE); (* UDP *)
  if sock = NL_INVALID then
  begin
    nlShutdown();
    Halt(1);
  end;
  (* set the destination address *)
  nlSetRemoteAddr(sock, addr);
  (* send the message *)
  nlWrite(sock, command, SizeOf(NLulong));
  (* read the reply *)
  count := nlRead(sock, buffer, SizeOf(buffer));
  if count > 0 then
    Writeln('Banner is: ', PChar(@buffer[4]));
  nlShutdown();
end.
