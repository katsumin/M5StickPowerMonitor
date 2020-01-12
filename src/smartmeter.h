#ifndef _SMARTMETER_H_
#define _SMARTMETER_H_
#include <Arduino.h>
#include <Udp.h>
#include "EchonetUdp.h"

class SmartMeter
{
public:
    inline long getPower() { return _power; };
    inline void setPower(long power) { _power = power; };
    inline short getCurrentR() { return _currentR; };
    inline void setCurrentR(int current) { _currentR = current; };
    inline short getCurrentT() { return _currentT; };
    inline void setCurrentT(int current) { _currentT = current; };

    void init(EchonetUdp *pUdp, const char *server, void (*callback)(SmartMeter *))
    {
        _server.fromString(server);
        _pEchonetUdp = pUdp;
        _pEchonetUdp->setCallback(_server, &parse, this);
        _callback = callback;
        _pSendUdp = new WiFiUDP();
    };
    void request()
    {
        Serial.print("request -> ");
        Serial.println(_server);
        // UDP *udp = _pEchonetUdp->getUdp();
        // udp->beginPacket(_server, 3610); // Echonet requests are to port 3610
        // udp->write(_cmd_buf, sizeof(_cmd_buf));
        // udp->endPacket();
        _pSendUdp->beginPacket(_server, 3610); // Echonet requests are to port 3610
        _pSendUdp->write(_cmd_buf, sizeof(_cmd_buf));
        _pSendUdp->endPacket();
    };

private:
    UDP *_pSendUdp;
    long _power;
    short _currentR;
    short _currentT;
    IPAddress _server;
    EchonetUdp *_pEchonetUdp = nullptr;
    void (*_callback)(SmartMeter *) = nullptr;
    u_char _cmd_buf[2 + 2 + 3 + 3 + 1 + 1 + 2 * (1 + 1)] = {
        0x10,
        0x81, // EHD
        0x00,
        0x01, // TID
        0x05,
        0xff,
        0x01, // SEOJ
        0x02,
        0x88,
        0x01, // DEOJ
        0x62, // ESV(プロパティ値読み出し要求)
        0x02, // OPC(2data)
        0xe7, // EPC()
        0x00, // PDC
        0xe8, // EPC()
        0x00, // PDC
    };
    void parseE7(u_char *edt)
    {
        long v = 0;
        for (int i = 0; i < 4; i++)
        {
            v <<= 8;
            v |= edt[i];
        }
        setPower(v);
    }
    void parseE8(u_char *edt)
    {
        setCurrentR(edt[0] << 8 | edt[1]);
        setCurrentT(edt[2] << 8 | edt[3]);
    }
    static void parse(ECHONET_FRAME *ef, void *obj)
    {
        SmartMeter *sm = (SmartMeter *)obj;
        ECHONET_DATA *pd = &ef->data;
        for (int i = 0; i < ef->opc; i++)
        {
            int s = pd->pdc;
            switch (pd->epc)
            {
            case 0xe7: // 瞬時電力
                sm->parseE7(pd->edt);
                break;
            case 0xe8: // 瞬時電流１
                sm->parseE8(pd->edt);
                break;
            }
            pd = (ECHONET_DATA *)(pd->edt + s);
        }
        sm->_callback(sm);
    };
};

SmartMeter smartmeter;
#endif