#ifndef TEMPERATURE
#define TEMPERATURE

class Temperature {
public:
  unsigned char value;
  Temperature();
  Temperature(float temp);
  operator float() const;
  operator bool() const;
};

#endif
