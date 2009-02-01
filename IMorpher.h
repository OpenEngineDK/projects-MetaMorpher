#ifndef _MORPHER_INTERFACE_
#define _MORPHER_INTERFACE_

template <class T>
class IMorpher {
 public:
  /** scaling between 0 and 1 */
  virtual void Morph(T* from, T* to, float scaling) = 0;
  virtual T* GetObject() = 0;
  virtual ~IMorpher() {}
};

#endif // _MORPHER_INTERFACE_
