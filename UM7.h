// UM7 classes header
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <iostream>
#include <cmath>
#include <bitset>
#include <cstdint>
#include <time.h>
#include <sstream>
#include <vector>

#define ANSI_COLOR_RED     "\x1b[31;1m"
#define ANSI_COLOR_GREEN   "\x1b[32;1m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35;1m"
#define ANSI_COLOR_CYAN    "\x1b[36;1m"
#define ANSI_COLOR_RESET   "\x1b[0m"

using namespace std;

//cout << "Got to line: " << __LINE__ << ", File: " << __FILE__ << endl;


class Packet
{
private:
public:
    int packet_type;
    int address;
    int length = 0;
    char *data;
    uint16_t checksum;
    void calculateLength ( Packet *received_packet );
    void getLength();
    Packet();
    ~Packet();
    void checkHealth();
    uint16_t generateChecksum();
};

class UM7
{
private:

    vector<string> register_name;


    int serial_handler;
    int error;
public:
    void findBeginning();
    void setError ( int err );
    int getFrequency();
    // int getIntegerInput ( int *input );

    int getPacketType();
    int getAddress();
    void echoPacket ( int iterations );
    int overwriteRegister ( int address, int byte_index );
    int packetRequest ( Packet *config_packet );
    int registerWrite ( int address, vector<string> *command );
    int confirmWrite ( int requested_address );
    int getRegister ( Packet *config_packet );

    UM7();
    ~UM7();
    void config();
};

////////////////////////////////
///////////////////////////// //
////////////////////////// // //
// Functions Definition // // //
////////////////////////// // //
///////////////////////////// //
////////////////////////////////

int getIntegerInput ( int *input )
{
    string line;
    cin >> line;
    istringstream s ( line );

    if ( ! ( s >> *input ) )
    {
        printf ( "\n" ANSI_COLOR_RED "Input was not a number" ANSI_COLOR_RESET );
        return -1;
    }

    char c;

    if ( s >> c )
    {
        printf ( "\n" ANSI_COLOR_RED "Input included non-numerical characters"
                 ANSI_COLOR_RESET );
        return -2;
    }

    return 0;
}

void UM7::setError ( int err )
{
    error = err;

    if ( error != 0 )
    {
        exit ( error );
    }
}

Packet::Packet() {};

Packet::~Packet()
{
    if ( length != 0 )
    {
        delete[] ( data );
    }
}

uint16_t Packet::generateChecksum()
{
    uint16_t computed_checksum;
    computed_checksum = 's' + 'n' + 'p';
    computed_checksum += packet_type + address;

    for ( int i = 0; i < length * 4; i++ )
    {
        computed_checksum += int ( uint8_t ( data[i] ) );
    }

    return computed_checksum;
}

void Packet::checkHealth()
{
    uint16_t computed_checksum = generateChecksum();

    if ( computed_checksum == checksum )
    {
        printf ( "\n" ANSI_COLOR_GREEN "Healthy\n" ANSI_COLOR_RESET );
    }
    else
    {
        printf ( "\n" ANSI_COLOR_RED "Corrupted" ANSI_COLOR_RESET );
    }
}

void Packet::getLength ()
{
    if ( length != 0 )
    {
        delete[] ( data );
    }

    length = 0;
    string binary = bitset<8> ( packet_type ).to_string();

    if ( binary[0] == '1' )
    {
        length = 1;
    }

    if ( binary[1] == '1' )
    {
        length = 0;
    }

    if ( binary[2] == '1' )
    {
        length += 8;
    }

    if ( binary[3] == '1' )
    {
        length += 4;
    }

    if ( binary[4] == '1' )
    {
        length += 2;
    }

    if ( binary[5] == '1' )
    {
        length += 1;
    }

    data = new char [length * 4];
    cout << endl <<
         "===========================================================================" <<
         endl;

    if ( binary[7] == '1' )
    {
        cout << "******COMMUNICATION FAILED******";
    }

    cout << packet_type << ": " << binary << ": ";

    if ( binary[0] == '1' )
    {
        cout << "has data.";
    }

    if ( binary[1] == '1' )
    {
        cout << "\b and is batch";
    }
}
UM7::UM7()
{
    printf ( ANSI_COLOR_CYAN
             "######################## UM7 Communication, BarzinM #######################\n"
             ANSI_COLOR_RESET );
    /* Open File Descriptor */
    serial_handler = open ( "/dev/ttyUSB0", O_RDWR | O_NOCTTY );

    /* Error Handling */
    if ( serial_handler < 0 )
    {
        cout << "Error " << errno << " opening " << "/dev/ttyUSB0" << ": " << strerror (
                 errno ) << endl;
    }

    /* *** Configure Port *** */
    struct termios tty;
    //memset ( &tty, 0, sizeof tty );

    /* Error Handling */
    if ( tcgetattr ( serial_handler, &tty ) != 0 )
    {
        cout << "Error " << errno << " from tcgetattr: " << strerror ( errno ) << endl;
    }

    /* Set Baud Rate */
    cfsetospeed ( &tty, ( speed_t ) B115200 );
    cfsetispeed ( &tty, ( speed_t ) B115200 );
    /* Setting other Port Stuff */
    tty.c_cflag &= ~PARENB;      // Make 8n1
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;     // no flow control
    tty.c_cc[VMIN] = 1;                // read doesn't block
    tty.c_cc[VTIME] = 5;                // 0.5 seconds read timeout
    tty.c_cflag |= CREAD | CLOCAL;    // turn on READ & ignore ctrl lines
    /* Make raw */
    cfmakeraw ( &tty );
    /* Flush Port, then applies attributes */
    tcflush ( serial_handler, TCIOFLUSH );

    if ( tcsetattr ( serial_handler, TCSANOW, &tty ) != 0 )
    {
        cout << "Error " << errno <<
             " from tcsetattr. Refer to http://pubs.opengroup.org/onlinepubs/7908799/xsh/termios.h.html"
             << endl;
    }

    setError ( 0 );
}

UM7::~UM7()
{
    close ( serial_handler );
    printf ( ANSI_COLOR_CYAN
             "++++++++++++++++++++++++ Exiting IMU Communication Program ++++++++++++++++\n"
             ANSI_COLOR_RESET );
}

void UM7::findBeginning()
{
    int n = 0;
    char buffer[10];

    while ( true )
    {
        // Find 's' character
        n = read ( serial_handler, buffer, 1 );

        if ( *buffer == 's' )
        {
            // Check to see if it is followed by 'n' and 'p'
            n = read ( serial_handler, buffer, 1 );

            if ( *buffer == 'n' )
            {
                n = read ( serial_handler, buffer, 1 );

                if ( *buffer == 'p' )
                {
                    return;
                }
            }
        }
    }
}

void UM7::echoPacket ( int iter )
{
    Packet received_pkt;
    uint16_t received_checksum;
    unsigned char decimal;
    string binary;
    char buffer[100];
    /* *** READ *** */
    int n;
    int buffer_value;
    /* Whole response*/
    tcflush ( serial_handler, TCIOFLUSH );

    for ( int i = 0; i < iter; i++ )
    {
        // computer_checksum = 's' + 'n' + 'p';
        findBeginning();
        n = read ( serial_handler, buffer, 1 );
        int temp = int ( uint8_t ( buffer[0] ) );
        received_pkt.packet_type = temp;
        // packet_type
        // computer_checksum += uint8_t ( buffer[0] );
        received_pkt.getLength ( );
        //char *binary_char = &binary[0];
        n = read ( serial_handler, buffer, 1 );
        received_pkt.address =  int ( uint8_t ( buffer[0] ) );
        printf ( "\nAddress is %i = %#x", received_pkt.address, received_pkt.address );
        cout << " = " << bitset<8> ( received_pkt.address ).to_string();
        cout << "\nData length = " << received_pkt.length << " registers";
        // else{cout<<endl<<"Skipped";}
        // if( address==86||address==112||address==97||address==85 ){continue;}//else{i=iter;}
        binary = bitset<8> ( received_pkt.address ).to_string(); //to binary

        for ( int i = 0; i < received_pkt.length * 4; i++ )
        {
            if ( i % 4 == 0 )
            {
                printf ( "\nReg %3i:  | ", received_pkt.address + i / 4 );
            }

            n = read ( serial_handler, buffer, 1 );
            received_pkt.data[i] =  int ( uint8_t ( buffer[0] ) );
            binary = bitset<8> ( buffer[0] ).to_string(); //to binary
            printf ( "%3u: ", ( uint8_t ( received_pkt.data[i] ) ) );
            cout << binary << " | ";
        }

        n = read ( serial_handler, buffer, 2 );
        received_pkt.checksum = uint16_t ( buffer[0] ) << 8 | uint8_t ( buffer[1] );
        received_pkt.checkHealth();
        // if ( n < 0 )
        // {
        //     cout << "Reading Error" << strerror ( errno ) << endl;
        //     break;
        // }
        // if ( n == 0 )
        // {
        //     cout << "Nothing to read";
        //     break;
        // }
        //
    }

    cout << endl;
    setError ( 0 );
}

void UM7::config()
{
    bool exitMode = 0;
    string binary;
    int input = -1;
    unsigned int address;
    int byte_index;
    int raw_rate = 0;

    while ( exitMode == 0 )
    {
        cout << "\nYou are in developer mode. Choose an option" << endl << endl;
        cout << "        ____________________________________________________________"
             << endl;
        cout << "        |    |                                                     |"
             << endl;
        cout << "        |  1 | Set ALL RAW SENSOR data transmission rate.          |"
             << endl;
        cout << "        |  2 | Set ALL PROCESSESED SENSOR data transmission rate.  |"
             << endl;
        cout << "        |  3 | Set ALL POSE data transmission rate.                |"
             << endl;
        cout << "        |____|_____________________________________________________|"
             << endl;
        cout << "        |    |                                                     |"
             << endl;
        cout << "        |  4 | Set QUATERNION data transmission rate.              |"
             << endl;
        cout << "        |  5 | Set EULER data transmission rate.                   |"
             << endl;
        cout << "        |  6 | Set POSITION data transmission rate.                |"
             << endl;
        cout << "        |  7 | Set VELOCITY data transmission rate.                |"
             << endl;
        cout << "        |____|_____________________________________________________|"
             << endl;
        cout << "        |    |                                                     |"
             << endl;
        cout << "        |  8 | Set RAW ACCELEROMETER data transmission rate.       |"
             << endl;
        cout << "        |  9 | Set RAW GYROSCOPE data transmission rate.           |"
             << endl;
        cout << "        | 10 | Set RAW MAGNETOMETER data transmission rate.        |"
             << endl;
        cout << "        |____|_____________________________________________________|"
             << endl;
        cout << "        |    |                                                     |"
             << endl;
        cout << "        | 11 | Set PROCESSED ACCELEROMETER data transmission rate. |"
             << endl;
        cout << "        | 12 | Set PROCESSED GYROSCOPE data transmission rate.     |"
             << endl;
        cout << "        | 13 | Set PROCESSED MAGENTOMETER data transmission rate.  |"
             << endl;
        cout << "        |____|_____________________________________________________|"
             << endl;
        cout << "        |    |                                                     |"
             << endl;
        cout << "        | 14 | Set TEMPERATURE data transmission rate.             |"
             << endl;
        cout << "        |____|_____________________________________________________|"
             << endl;
        cout << "        |    |                                                     |"
             << endl;
        cout << "        |  0 |EXIT.                                                |"
             << endl;
        cout << "        |____|_____________________________________________________|"
             << endl;
        cout << endl << ">> ";
        int error = getIntegerInput ( &input );

        if ( error != 0 )
        {
            cout << "\nBad input try again" << endl;
            continue;
        }

        switch ( input )
        {
        default:
            cout << "\nBad input try again" << endl;

        case 0:
            exitMode = 1;
            break;

        case 1:
            address = 2;
            byte_index = 3;

        case 2:
            address = 4;
            byte_index = 3;

        case 3:
            address = 6;
            byte_index = 0;

        case 4:
            address = 5;
            byte_index = 0;

        case 5:
            address = 5;
            byte_index = 1;

        case 6:
            address = 5;
            byte_index = 2;

        case 7:
            address = 5;
            byte_index = 3;

        case 8:
            address = 1;
            byte_index = 0;

        case 9:
            address = 1;
            byte_index = 1;

        case 10:
            address = 1;
            byte_index = 2;

        case 11:
            address = 3;
            byte_index = 0;

        case 12:
            address = 3;
            byte_index = 1;

        case 13:
            address = 3;
            byte_index = 2;

        case 14:
            address = 2;
            byte_index = 0;
        }

        if ( input != 0 )
        {
            // insertByte ( serialCommunicationHandler, address, byte_index );
        }
    }

    cout << "You are exiting developer mode in 3 seconds.\n3..." << endl;
    sleep ( 1 );
    cout << "2..." << endl;
    sleep ( 1 );
    cout << "1..." << endl;
    sleep ( 1 );
    return;
}

int UM7::packetRequest ( Packet *config_packet )
{
    // Start of packet
    char cmd[7];
    uint16_t ckecksum;
    // vector<string> bytes_in_binary;
    int error = 1;
    // Set packet type
    char packet_type = ( bitset<8> ( "00000000" ).to_ulong() );
    // Construct the command
    cmd[0] = 's';
    cmd[1] = 'n';
    cmd[2] = 'p';
    cmd[3] = packet_type;
    cmd[4] = char ( config_packet->address );
    ckecksum = 's' + 'n' + 'p' + uint8_t ( cmd[3] ) + uint8_t ( cmd[4] );
    cmd[5] = uint8_t ( ckecksum >> 8 );
    cmd[6] = uint8_t ( ckecksum );
    config_packet->packet_type = 128;
    ( *config_packet ).getLength();

    while ( error != 0 )
    {
        tcflush ( serial_handler, TCIOFLUSH );
        write ( serial_handler, cmd, 7 );
        error = getRegister ( config_packet );
    }

    return 0;
}

int UM7::getRegister ( Packet *config_packet )
{
    Packet temp_packet;
    string single_byte;
    bool found = 0;
    uint16_t received_checksum;
    //     /* *** READ *** */
    int n = 0;
    int iter;
    //     /* Whole response*/
    char *buffer = new char [100];
    iter = 10;

    for ( int i = 0; i < iter; i++ )
    {
        // Find 's' character
        findBeginning();
        n = read ( serial_handler, buffer, 1 );
        temp_packet.packet_type = int ( uint8_t ( buffer[0] ) );
        temp_packet.getLength();
        n = read ( serial_handler, buffer, 1 );
        temp_packet.address = int ( uint8_t ( buffer[0] ) );

        if ( config_packet->address != temp_packet.address )
        {
            read ( serial_handler, buffer, ( temp_packet.length * 4 ) + 2 );
            continue;
        }

        printf ( "\nReg %3i:  | ", config_packet->address );

        for ( int i = 0; i < ( config_packet->length * 4 ); i++ )
        {
            n = read ( serial_handler, buffer, 1 );
            config_packet->data[i] = *buffer;
            config_packet->data[i] = *buffer;
            single_byte = bitset<8> ( int (
                                          config_packet->data[i] ) ).to_string(); //to binary
            printf ( "%3i: ", int ( uint8_t ( buffer[0] ) ) );
            cout << single_byte << " | ";
        }

        n = read ( serial_handler, buffer, 2 );
        received_checksum = uint16_t ( buffer[0] ) << 8 | uint8_t ( buffer[1] );

        if ( ( *config_packet ).generateChecksum() ==
                received_checksum ) // use checkhealth funciton
        {
            printf ( "\n" ANSI_COLOR_GREEN "Healthy\n" ANSI_COLOR_RESET );

            if ( config_packet->address == temp_packet.address )
            {
                found = 1;
                delete[] ( buffer );
                return 0;
            }
        }
        else
        {
            printf ( "\n" ANSI_COLOR_RED "Corrupted" ANSI_COLOR_RESET );
        }

        if ( n < 0 )
        {
            cout << "Reading Error" << strerror ( errno ) << endl;
            delete[] ( buffer );
            return -1;
        }

        if ( n == 0 )
        {
            cout << "Nothing to read";
            delete[] ( buffer );
            return -2;
        }
    }

    if ( !found )
    {
        printf ( ANSI_COLOR_RED "Did NOT get the requested register information"
                 ANSI_COLOR_RESET );
    }

    cout << endl;
    delete[] ( buffer );
    return -3;
}