AC5 =  24847
AC6 =  21549
MC  = -11786
MD  =  2913

raw = hass.states.get('sensor.bmp180')
if raw and raw.state not in ('unknown', 'unavailable'):
    UT = int(float(raw.state))

    X1 = ((UT - AC6) * AC5) >> 15
    X2 = (MC << 11) // (X1 + MD)
    B5 = X1 + X2
    T  = (B5 + 8) >> 4
    temperature_c = round(T / 10.0, 2)

    hass.states.set('sensor.bmp180', temperature_c, {
        'unit_of_measurement': '°C',
        'device_class': 'temperature'
    })