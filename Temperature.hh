#ifndef TEMPERATURE_HH
#define TEMPERATURE_HH

class Temperature {
  unsigned char value;
public:
  Temperature();
  Temperature(float temp);
  operator float() const;
  operator bool() const;
};

#endif
