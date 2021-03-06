/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef example_TYPES_H
#define example_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>




class FeatureVector;

class Example;

typedef struct _FeatureVector__isset {
  _FeatureVector__isset() : id(false), stringFeatures(false), floatFeatures(false), denseFeatures(false) {}
  bool id :1;
  bool stringFeatures :1;
  bool floatFeatures :1;
  bool denseFeatures :1;
} _FeatureVector__isset;

class FeatureVector {
 public:

  FeatureVector(const FeatureVector&);
  FeatureVector(FeatureVector&&);
  FeatureVector& operator=(const FeatureVector&);
  FeatureVector& operator=(FeatureVector&&);
  FeatureVector() : id() {
  }

  virtual ~FeatureVector() throw();
  std::string id;
  std::map<std::string, std::set<std::string> >  stringFeatures;
  std::map<std::string, std::map<std::string, double> >  floatFeatures;
  std::map<std::string, std::vector<double> >  denseFeatures;

  _FeatureVector__isset __isset;

  void __set_id(const std::string& val);

  void __set_stringFeatures(const std::map<std::string, std::set<std::string> > & val);

  void __set_floatFeatures(const std::map<std::string, std::map<std::string, double> > & val);

  void __set_denseFeatures(const std::map<std::string, std::vector<double> > & val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(FeatureVector &a, FeatureVector &b);

inline std::ostream& operator<<(std::ostream& out, const FeatureVector& obj)
{
  obj.printTo(out);
  return out;
}

typedef struct _Example__isset {
  _Example__isset() : example(false), context(false), metadata(false) {}
  bool example :1;
  bool context :1;
  bool metadata :1;
} _Example__isset;

class Example {
 public:

  Example(const Example&);
  Example(Example&&);
  Example& operator=(const Example&);
  Example& operator=(Example&&);
  Example() {
  }

  virtual ~Example() throw();
  std::vector<FeatureVector>  example;
  FeatureVector context;
  std::map<std::string, std::string>  metadata;

  _Example__isset __isset;

  void __set_example(const std::vector<FeatureVector> & val);

  void __set_context(const FeatureVector& val);

  void __set_metadata(const std::map<std::string, std::string> & val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(Example &a, Example &b);

inline std::ostream& operator<<(std::ostream& out, const Example& obj)
{
  obj.printTo(out);
  return out;
}



#include "example_types.tcc"

#endif
