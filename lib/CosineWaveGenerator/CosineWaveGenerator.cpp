#include "CosineWaveGenerator.h"
typedef struct { double freq; double fDelta; int step; } FandStep;

void CosineWaveGenerator::enable(dac_channel_t channel)
{
    // Enable tone generator common to both channels
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
    switch(channel) 
    {
        case DAC_CHANNEL_1:
            SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, (int)_mode[0], SENS_DAC_INV1_S);
            dac_output_enable(DAC_CHANNEL_1);
            _enabled[(int)channel - 1] = true;
        break;
        case DAC_CHANNEL_2:
            SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, (int)_mode[1], SENS_DAC_INV2_S);
            dac_output_enable(DAC_CHANNEL_2);
            _enabled[(int)channel - 1] = true;
        break;
        default :
           printf("Wrong channel number %d\n", channel);
        break;
    }
}

void CosineWaveGenerator::disable(dac_channel_t channel)
{
    // Enable tone generator common to both channels
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
    switch(channel) 
    {
        case DAC_CHANNEL_1:
            SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, (int)_mode[0], SENS_DAC_INV1_S);
            dac_output_disable(DAC_CHANNEL_1);
            _enabled[(int)channel - 1] = false;
        break;
        case DAC_CHANNEL_2:
            SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, (int)_mode[1], SENS_DAC_INV2_S);
            dac_output_disable(DAC_CHANNEL_2);
            _enabled[(int)channel - 1] = false;
        break;
        default :
           printf("Wrong channel number %d\n", channel);
        break;
    }
}

bool CosineWaveGenerator::isEnabled(dac_channel_t channel)
{
    return _enabled[(int)channel - 1];
}

void CosineWaveGenerator::toggle(dac_channel_t channel)
{
    isEnabled(channel) ? disable(channel) : enable(channel);
}

void CosineWaveGenerator::setScale(dac_channel_t channel, int scale)
{
    switch(channel) 
    {
        case DAC_CHANNEL_1:
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE1, scale, SENS_DAC_SCALE1_S);
            _scale[0] = scale;
        break;
        case DAC_CHANNEL_2:
            SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE2, scale, SENS_DAC_SCALE2_S);
            _scale[1] = scale;
        break;
        default :
            printf("Wrong channel number %d\n", channel);
        break;
    } 
}

void CosineWaveGenerator::setOffset(dac_channel_t channel, int offset)
{
    switch(channel) 
    {
    case DAC_CHANNEL_1:
        SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DC1, offset, SENS_DAC_DC1_S);
        _offset[0] = offset;
    break;
    case DAC_CHANNEL_2:
        SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DC2, offset, SENS_DAC_DC2_S);
        _offset[1] = offset;
    break;
    default :
        printf("Wrong channel number %d\n", channel);
    break;
    }
}

void CosineWaveGenerator::setMode(dac_channel_t channel, CWmode mode)
{
    switch(channel) 
    {
    case DAC_CHANNEL_1:
        SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, (int)mode, SENS_DAC_INV1_S);
        _mode[0] = mode;
    break;
    case DAC_CHANNEL_2:
        SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, (int)mode, SENS_DAC_INV2_S);
        _mode[1] = mode;
    break;
    default :
        printf("Wrong channel number %d\n", channel);
    break;
    }
}

void CosineWaveGenerator::setFrequency(int clk_8m_div, int frequencyStep)
{
    REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, clk_8m_div);
    SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, frequencyStep, SENS_SW_FSTEP_S);
    _divi = clk_8m_div;
    _step = frequencyStep;
    _f_actual = _f0 * _step / (1 + _divi);
    _f_delta  = _f_actual - _f_target;    
}

void CosineWaveGenerator::setClockDivisor(int clk_8m_div)
{
    REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, clk_8m_div);
    _divi = clk_8m_div;
    _f_actual = _f0 * _step / (1 + _divi);
}

int CosineWaveGenerator::getClockDivisor()
{
    return _divi;
}

void CosineWaveGenerator::setFrequencyStep(int frequencyStep)
{
    SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, frequencyStep, SENS_SW_FSTEP_S);
    _step = frequencyStep;
    _f_actual = _f0 * _step / (1 + _divi);
}

int CosineWaveGenerator::getFrequencyStep()
{
    return _step;
}

/**
 * f = f0 * step / (divi + 1)
 */
void CosineWaveGenerator::setFrequencyWithDivisor(double f, int divi)
{
    if (divi < 0) divi = 0;
    if (divi > 7) divi = 7;
    _f_target = f;
    _divi = divi;
    _step = (int)(round(_f_target * (_divi + 1) / _f0));
    setFrequency(_divi, _step);
}

void CosineWaveGenerator::setFrequencyWithStep(double f, int step)
{
    if (step < 1) step = 1;
    if (step > 0xffff) step = 0xffff;
    _f_target = f;
    _step = step;
    _divi = (int)round(_f0 * step / _f_target) - 1;
    if (_divi < 1) _divi = 1;
    setFrequency(_divi, _step);
}

/**
 * f = f0 * step / (divi + 1)
 * For a given target frequency f_target, we compute the value q = f_target / f0
 * Now we calculate for all possible divisors 1..7 the integer step value and take
 * the divi/step pair with which we get the closest approximation for f_target.
 * To get a smooth wave form it is desirable to take a low divisor value and to 
 * tolerate a larger frequency deviation. The allowed frequency tolerance can be
 * set with the method setToleranceForBestMatch()
 */
void CosineWaveGenerator::setFrequency(double ft)
{
    FandStep f_s[8];
    
    _f_target = ft;

    double q = _f_target / _f0;

    for (int i = 0; i < 8; i++)  // get frequencies and step for all divisors
    {
        f_s[i].step = int(round(q * (i + 1)));          // keep step
        f_s[i].freq = (_f0 * f_s[i].step) / (i + 1);    // keep resulting frequency      
    }

    double fDelta = _f_target;
    double fd    = _f_target;
    int i_best   = 9;   // initialize to invalid value
    int i_lowest = 9;
    for (int i = 0; i < 8; i++)  // find frequency within tolerance and best matcha3
    {
        fd = abs(_f_target - f_s[i].freq);
        if (fd < fDelta)
        {
            i_best = i;
            f_s[i_best].fDelta = fd;
            fDelta = fd;
        }
        if (fd < _f_target * _f_tolerance / 1000.0) 
        {
            if (i_lowest == 9) i_lowest = i;
        }   
    }

    printf("Frequency with best match %.2f, divisor = %d, step = %d\n", f_s[i_best].freq, i_best, f_s[i_best].step);
    if (i_lowest == 9)
    {
        printf("Frequency with given tolerance of %d °/oo cannot be set.\n", _f_tolerance);
        setFrequency(i_best, f_s[i_best].step);
        printf("Best approx set instead: f_actual = %.2f, delta = %.2f, divisor = %d, step = %d\n", _f_actual, f_s[i_best].fDelta, i_best, f_s[i_best].step);
    }  
    else
    {
        printf("Frequency within tolerance of %d °/oo is %.2f, delta %.2f\n", _f_tolerance, f_s[i_lowest].freq, f_s[i_lowest].fDelta);
        setFrequency(i_lowest, f_s[i_lowest].step);
    }  
} 

double CosineWaveGenerator::getActualFrequency()
{
    return _f0 * _step / (1 + _divi); 
}

void CosineWaveGenerator::setReferenceFrequency(double f0)
{
    _f0 = f0;
    _f_actual = _f0 * _step / (1 + _divi); 
    _f_delta = _f_actual - _f_target;
}

void CosineWaveGenerator::setToleranceForBestMatch(int tolerance)
{
    _f_tolerance = tolerance;
    if (_f_tolerance < 1)   _f_tolerance = 1;
    if (_f_tolerance > 999) _f_tolerance = 999;
}

void CosineWaveGenerator::printCwgData()
{
    printf("\nf0          = %9.2f\n", _f0);
    printf("step        = %9d\n", _step);
    printf("divi        = %9d\n", _divi);
    printf("f_tolerance = %9d °/oo\n", _f_tolerance);
    printf("f_target    = %9.2f\n", _f_target);
    printf("f_actual    = %9.2f\n", _f_actual);
    printf("f_delta     = %9.2f\n", _f_delta);
}