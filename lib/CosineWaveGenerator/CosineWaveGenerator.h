#include <Arduino.h>
 
#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"
 
#include "driver/dac.h"

enum class CWmode { CW_M_W, CW_W_M, CW_SINE, CW_NEG_SINE };

class CosineWaveGenerator
{
    public:
        CosineWaveGenerator(double f0) : _f0(f0)  // initialize f0 to a measured reference frequency
        { 
            _scale[0] = 0;
            _scale[1] = 0;
            _offset[0] = 0;
            _offset[1] = 0;
            _mode[0] = CWmode::CW_SINE;
            _mode[1] = CWmode::CW_SINE;
            _enabled[0] = false;
            _enabled[1] = false;
            _f_actual = _f0 *_step / (_divi + 1);
            _f_target = _f_actual;
            _f_delta  = _f_target - _f_actual;
            _f_tolerance = 10.0;
            setScale(DAC_CHANNEL_1, _scale[0]);
            setScale(DAC_CHANNEL_2, _scale[1]);
            setOffset(DAC_CHANNEL_1, _offset[0]);
            setOffset(DAC_CHANNEL_2, _offset[1]);
            setFrequency(_divi, _step);
        }

        void enable(dac_channel_t channel);
        void disable(dac_channel_t channel);
        void toggle(dac_channel_t channel);
        bool isEnabled(dac_channel_t channel);
        void setScale(dac_channel_t channel, int scale);
        void setOffset(dac_channel_t channel, int offset);
        void setMode(dac_channel_t channel, CWmode mode);
        void setFrequency(int clk_8m_div, int frequency_step);
        void setFrequency(double f, double tolerance);
        void setFrequency(double f);
        void setFrequencyWithDivisor(double f, int clk_8m_div);
        void setFrequencyWithStep(double f, int step);
        double getActualFrequency();
        void setReferenceFrequency(double f0);
        void setClockDivisor(int clk_8m_div);
        int  getClockDivisor();
        void setFrequencyStep(int frequencyStep);
        int  getFrequencyStep();
        void setToleranceForBestMatch(int tolerance);
        void printCwgData();

    private:
        double _f0;                  // frequency generated with step = 1 and divi = 0;
        double _f_target;            // desired frequency
        double _f_actual;            // actual frequency generated
        double _f_delta;             // frequency deviation from f_target
        int    _f_tolerance = 10;    // frequency deviation in per thousend (1..999)
        int    _step = 1;            // 1..65535
        int    _divi = 0;            // 0..7
        int    _scale[2];            // 0..3   Vout * (2^0, 2^-1, 2^-2, 2^-3)
        int    _offset[2];           // 0..255
        bool   _enabled[2];          // CHN_1 or CHN_2 enabled=true, disabled=false
        CWmode _mode[2];   
};