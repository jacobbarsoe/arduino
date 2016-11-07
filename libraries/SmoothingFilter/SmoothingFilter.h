#ifndef _SMOOTHING_FILTER_
#define _SMOOTHING_FILTER_
// euclidian smoothing filter - .99 very slow filter
class SmoothingFilter
{
 private:
  double alphaOld;  
  double alphaNew;
  double output;
public:
  SmoothingFilter(double timeconst) :
	alphaOld(timeconst),
	alphaNew(1-timeconst),
	output(0.0)
    {
    };
  
  void init(double initValue)
  {
    output = initValue;
  };

  void newSample(double value)
  {
    output = alphaOld*output + alphaNew*value;
  };

  double getFilteredValue() { return output; };
};
#endif
