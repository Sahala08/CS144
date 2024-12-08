#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  // cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  TCPSocket tcpsock {};
  tcpsock.connect(Address(host, "http"));

  // 发送 HTTP GET 请求
  string request = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  // cerr << request << endl;
  tcpsock.write(request);

  // tcpsock.write( string_view( "GET " + path + " HTTP/1.1\r\n" ) );
  // tcpsock.write( string_view( "Host: " + host + "\r\n" ) );
  // tcpsock.write( string_view( "Connection: close\r\n\r\n" ) );

  // 关闭写入方向
  tcpsock.shutdown(SHUT_WR);

  // 读取数据  
  while( not tcpsock.eof()) {
    string buffer;
    tcpsock.read(buffer);
    cout << buffer;
  }
  // cerr << endl;
  
  // 关闭套接字
  tcpsock.close();
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
