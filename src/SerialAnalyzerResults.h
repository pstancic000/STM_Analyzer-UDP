#ifndef SERIAL_ANALYZER_RESULTS
#define SERIAL_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define FRAMING_ERROR_FLAG ( 1 << 0 )
#define PARITY_ERROR_FLAG ( 1 << 1 )
#define MP_MODE_ADDRESS_FLAG ( 1 << 2 )

class SerialAnalyzer;
class SerialAnalyzerSettings;

class SerialAnalyzerResults : public AnalyzerResults
{
  public:
    SerialAnalyzerResults( SerialAnalyzer* analyzer, SerialAnalyzerSettings* settings );
    virtual ~SerialAnalyzerResults();

    virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
    virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

    virtual void GenerateFrameTabularText( U64 frame_index, DisplayBase display_base );
    virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
    virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

  protected:  // functions
    void Parse_Data_STM(char number_str[]);
    void Print_Command_STM(int CMD_ID_Value);
    void SendUDPMessage(const void* data, size_t length, const char* ip, int port);

    void ConcatBuffers(const char* SourceBuffer, char* DestinationBuffer);
    void CleanBuffer(char* Buffer);
    void CopyStringCommand_STM(int CMD_ID_Value, char* Buffer);
    // For converting cut hex strings to bytes for sending via UDP
    void ConvertHexStringToBytes(const char* hexString, std::vector<uint8_t>& byteArray);
    void ConvertHexStringToBytes3(const char* hexString, std::vector<uint8_t>& byteArray);

    // Chat GPT Pokusaj zamene ConcatBuffer
    void AppendToBuffer(const char* source, char* destination, size_t& currentLength, size_t maxLength);

  protected: // vars
    SerialAnalyzerSettings* mSettings;
    SerialAnalyzer* mAnalyzer;
};

#endif // SERIAL_ANALYZER_RESULTS
