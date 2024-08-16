#ifndef SERIAL_ANALYZER_SETTINGS
#define SERIAL_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

#include <memory>

namespace SerialAnalyzerEnums
{
    enum Mode
    {
        Normal,
        MpModeMsbZeroMeansAddress,
        MpModeMsbOneMeansAddress
    };
}

class SerialAnalyzerSettings : public AnalyzerSettings
{
  public:
    SerialAnalyzerSettings();
    virtual ~SerialAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();

    // Getters for UDP settings
    U32 GetPortNumber() const
    {
        return mPortNumber;
    }
    const char* GetIPAddressString() const
    {
        return mIPAddressString;
    }
    double GetBand() const
    {
        return mBandSelect;
    }
    double GetPacketType() const
    {
        return mPacketType;
    }


    Channel mInputChannel;
    U32 mBitRate;
    U32 mBitsPerTransfer;
    AnalyzerEnums::ShiftOrder mShiftOrder;
    double mStopBits;
    AnalyzerEnums::Parity mParity;
    bool mInverted;
    bool mUseAutobaud;
    SerialAnalyzerEnums::Mode mSerialMode;

    // Added for UDP
    U32 mPortNumber;
    const char* mIPAddressString;
    double mBandSelect;
    double mPacketType;

  protected:
    std::unique_ptr<AnalyzerSettingInterfaceChannel> mInputChannelInterface;
    std::unique_ptr<AnalyzerSettingInterfaceInteger> mBitRateInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mBitsPerTransferInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mShiftOrderInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mStopBitsInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mParityInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mInvertedInterface;
    std::unique_ptr<AnalyzerSettingInterfaceBool> mUseAutobaudInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mSerialModeInterface;
    // Added for UDP
    std::unique_ptr<AnalyzerSettingInterfaceText> mSerialModeInterface_IP; // AnalyzerSettingInterfaceText TREBA DA BUDE
    std::unique_ptr<AnalyzerSettingInterfaceInteger> mSerialModeInterface_PORT;
    // ADded for header when sending with UDP
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mBandSelectedInterface; // Mozda treba nekako integer umesto ovog, // Band
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mPacketTypeInterface;   // Sniffer ili IPV6
};

#endif // SERIAL_ANALYZER_SETTINGS
