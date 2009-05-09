//
// Quaternion
//
// written by Martin Reuter
// Nov. 4th ,2008
//

#ifndef Quaternion_H
#define Quaternion_H

#include <cmath>
#include <iostream>
#include <vector>

class Quaternion;
std::ostream& operator<<(std::ostream& os, const Quaternion& q);

class Quaternion
{
public:

  Quaternion(double ta, double tb, double tc, double td, bool tnormed = false):
      a(ta), b(tb), c(tc),d(td),normed(tnormed)
  {};
//     Quaternion(double v[4], bool tnormed = false):
//           a(v[0]), b(v[1]), c(v[2]),d(v[3]),normed(tnormed) {};
//     Quaternion(double real, double img[3], bool tnormed = false):
//           a(real), b(img[0]), c(img[1]),d(img[2]),normed(tnormed) {};
  Quaternion():
      a(1.0), b(0.0), c(0.0),d(0.0),normed(true)
  {};

  inline double            getReal() const
  {
    return a;
  };
  inline std::vector < double > getImg() const
  {
    std::vector <double> v(3);
    v[0] = b;
    v[1] = c;
    v[2] = d;
    return v;
  };
  inline std::vector < double > getMatrix4d() const;

  inline void write(std::ostream& os) const;

  // Rotation stuff
  inline std::vector < double >  getRotMatrix3d() const;
  inline std::vector < double >  getRotMatrix3dh() const;
  inline std::vector < double >  getRotAxis() const;
  inline double      getRotAngle() const;
  inline Quaternion  getHalfRotation() const;
  inline Quaternion& importRotVec(double v1, double v2, double v3);
  inline Quaternion& importRotVec(double alpha, double v1, double v2, double v3);
  inline Quaternion& importZXZAngles(double z1, double x2, double z3);
  inline Quaternion& importXYZAngles(double x1, double y2, double z3);
  inline Quaternion& importZYXAngles(double z1, double y2, double x3);
  inline Quaternion& importZYXmAngles(double z1, double y2, double x3);
  inline Quaternion& importMatrix(double a00,double a01,double a02,
                                  double a10,double a11,double a12,
                                  double a20,double a21,double a22 );
  inline std::vector < double >   rotate(double v1, double v2, double v3 ) const;

  // Operations
  inline Quaternion  getConjugate() const;
  inline Quaternion& conjugate();
  inline Quaternion  getInverse() const;
  inline Quaternion& invert();
  inline double      norm() const;
  inline double      norm2() const;
  inline Quaternion& normalize();
  inline Quaternion  getNormalized() const;
  inline bool        isNormalized() const
  {
    return normed;
  };

  // Operators
  inline Quaternion& operator= (const Quaternion&);
  inline int         operator==(const Quaternion&) const;
  inline int         operator!=(const Quaternion&) const;
  inline Quaternion& operator+=(const Quaternion&);
  inline Quaternion& operator-=(const Quaternion&);
  inline Quaternion& operator*=(const double&);
  inline Quaternion  operator+ (const Quaternion&) const;
  inline Quaternion  operator- (const Quaternion&) const;
  inline Quaternion  operator* (const double&)     const;
  inline Quaternion  operator* (const Quaternion&) const;


private:
  double a,b,c,d;
  bool normed;

};

inline void Quaternion::write(std::ostream& os) const
{
  os << "( " << a << " , " << b << " , " << c << " , " << d << " )";
}

inline std::vector < double > Quaternion::getMatrix4d() const
// returns m0  m1  m2  m3
//         m4  m5  m6  m7
//         m8  m9  m10 m11
//         m12 m13 m14 m15
{
  std::vector < double > m(16);
  m[ 0] =  a;
  m[ 1] =  b;
  m[ 2] =  c;
  m[ 3] =  d;
  m[ 4] = -b;
  m[ 5] =  a;
  m[ 6] = -d;
  m[ 7] =  c;
  m[ 8] = -c;
  m[ 9] =  d;
  m[10] =  a;
  m[11] = -b;
  m[12] = -d;
  m[13] = -c;
  m[14] =  b;
  m[15] =  a;
  return m;
}

inline std::vector < double > Quaternion::getRotMatrix3d() const
// returns m0  m1  m2
//         m3  m4  m5
//         m6  m7  m8
{
  Quaternion q = getNormalized();

  double q11 = q.a *q.a;
  double q12 = q.a *q.b;
  double q13 = q.a *q.c;
  double q14 = q.a *q.d;
  double q22 = q.b *q.b;
  double q23 = q.b *q.c;
  double q24 = q.b *q.d;
  double q33 = q.c *q.c;
  double q34 = q.c *q.d;
  double q44 = q.d *q.d;

  std::vector < double > m(9);
  m[ 0] = q11+q22-q33-q44;
  m[ 1] = 2.0*(q23-q14);
  m[ 2] = 2.0*(q24+q13);
  m[ 3] = 2.0*(q23+q14);
  m[ 4] = q11-q22+q33-q44;
  m[ 5] = 2.0*(q34-q12);
  m[ 6] = 2.0*(q24-q13);
  m[ 7] = 2.0*(q34+q12);
  m[ 8] = q11-q22-q33+q44;

  return m;
}

inline std::vector < double > Quaternion::getRotMatrix3dh() const
// returns m0  m1  m2  0
//         m4  m5  m6  0
//         m8  m9  m10 0
//          0   0   0  1
// m[16] in homogeneous coordinates
{
  std::vector < double > mr = getRotMatrix3d();
  std::vector < double > m(16);
  m[ 0] = mr[0];
  m[ 1] = mr[1];
  m[ 2] = mr[2];
  m[ 3] = 0;
  m[ 4] = mr[3];
  m[ 5] = mr[4];
  m[ 6] = mr[5];
  m[ 7] = 0;
  m[ 8] = mr[6];
  m[ 9] = mr[7];
  m[10] = mr[8];
  m[11] = 0;
  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
  return m;

}

inline double   Quaternion::getRotAngle() const
{
//   double mya = a;
//   if (a>1) mya = 1;
//   if (a<-1) mya = -1;
//   if (normed) return 2.0 * acos(a);
  double l = sqrt(a*a+b*b+c*c+d*d); // more stable
  return 2.0 * acos(a/l);
}

inline Quaternion  Quaternion::getHalfRotation() const
{
  //std::cout << " Quaternion::getHalfRotation" << std::endl;
  //if (normed)   std::cout << " is normed " << std::endl;
  //else std::cout << " is not normed " << std::endl;
  double angle = .5 * getRotAngle();
  std::vector < double > v = getRotAxis();
  //std::cout << " angle: " << angle << " v: " << v[0] << " " << v[1] << " " <<v[2] << std::endl;
  Quaternion q;
  q.importRotVec(angle,v[0],v[1],v[2]);
  return q;
}

inline std::vector < double > Quaternion::getRotAxis() const
{
  std::vector < double > v(3);
  v[0] = b;
  v[1] = c;
  v[2] = d;
  double l = sqrt(b*b+c*c+d*d);
  l = 1.0/l;
  //std::cout << " l : " << l << std::endl;
  v[0] *= l;
  v[1] *= l;
  v[2] *= l;
  if (std::isnan(v[0]) || std::isnan(v[1]) || std::isnan(v[2]) )
  {
    v[0] = v[1] = v[2] = 0;
  }
  return v;
}

inline Quaternion& Quaternion::importRotVec(double v1, double v2, double v3)
// converts rotation vector v1,v2,v3 to quaternion
// rotation of ||v|| around axis defined by v
{
  normed = true;
  double l = sqrt(v1*v1+v2*v2+v3*v3);
  if (l < 0.000001)
  {
    a = 1.0;
    b = c = d = 0.0;
    return *this;
  }
  double slh = sin(l/2.0);
  a =  cos(l/2.0);
  b =  slh * v1 / l;
  c =  slh * v2 / l;
  d =  slh * v3 / l;
  return *this;
}

inline Quaternion& Quaternion::importRotVec(double alpha, double v1, double v2, double v3)
// converts rotation of alpha around axis vec to quaternion
{
  normed = true;
  //std::cout << alpha << " " << v1 << " " << v2 << " " << v3 << std::endl;
  double l = sqrt(v1*v1+v2*v2+v3*v3); // make sure vec is normalized
  if (l < 0.000001)
  {
    a = 1;
    b = c = d = 0.0;
    return *this;
  }

  double slh = sin(alpha/2.0);
  a =  cos(alpha/2.0);
  b =  slh * v1 / l;
  c =  slh * v2 / l;
  d =  slh * v3 / l;
  return *this;
}

inline Quaternion& Quaternion::importZXZAngles(double z1, double x2, double z3)
// converts rotation of z1 around z, x2 around x and z3 around z to quaternion
// Oreder 1. z, 2. x, 3. z (counterclockwise when looking along the axes)
// This is one of the Euler Angle definitions
{
  normed = true;

  Quaternion qz1;
  qz1.importRotVec(z1,0,0,1);
  Quaternion qx2;
  qx2.importRotVec(x2,1,0,0);
  Quaternion qz3;
  qz3.importRotVec(z3,0,0,1);

  this->operator=(qz3*qx2*qz1);

  return *this;
}

inline Quaternion& Quaternion::importXYZAngles(double x1, double y2, double z3)
// converts rotation of x1 around x, y2 around y and z3 around z to quaternion
// order 1. x, 2. y, 3. z (counterclockwise when looking along the axes)
// This is one of the Euler Angle definitions
{
  normed = true;

  Quaternion qx1;
  qx1.importRotVec(x1,1,0,0);
  Quaternion qy2;
  qy2.importRotVec(y2,0,1,0);
  Quaternion qz3;
  qz3.importRotVec(z3,0,0,1);

  this->operator=(qz3*qy2*qx1);

  return *this;
}

inline Quaternion& Quaternion::importZYXAngles(double z1, double y2, double x3)
// converts rotation of z1 around z, y2 around y and x3 around x to quaternion
// order 1. z, 2. y, 3. x (counterclockwise when looking along the axes)
// This is one of the Euler Angle definitions
{
  normed = true;

  Quaternion qz1;
  qz1.importRotVec(z1,0,0,1);
  Quaternion qy2;
  qy2.importRotVec(y2,0,1,0);
  Quaternion qx3;
  qx3.importRotVec(x3,1,0,0);

  this->operator=(qx3*qy2*qz1);

  return *this;
}

inline Quaternion& Quaternion::importZYXmAngles(double z1, double y2, double x3)
// converts rotation of z1 around -z, y2 around -y and x3 around -x to quaternion
// order 1. -z, 2. -y, 3. -x (clockwise when locking along x,y or z)
// This is one of the Euler Angle definitions
{
  normed = true;

  Quaternion qz1;
  qz1.importRotVec(-z1,0,0,1);
  Quaternion qy2;
  qy2.importRotVec(-y2,0,1,0);
  Quaternion qx3;
  qx3.importRotVec(-x3,1,0,0);

  this->operator=(qx3*qy2*qz1);

  return *this;
}

inline Quaternion& Quaternion::importMatrix(double a00,double a01,double a02,
    double a10,double a11,double a12,
    double a20,double a21,double a22 )
{
  double trace = a00 + a11 + a22 + 1.0;

  if ( trace > 0.00001)
  {
    double s = 0.5 / sqrt(trace);
    a = 0.25 / s;
    b = ( a21 - a12 ) * s;
    c = ( a02 - a20 ) * s;
    d = ( a10 - a01 ) * s;
  }
  else
  {
    if ( a00 > a11 && a00 > a22 )
    {
      double s = 2.0 * sqrt( 1.0 + a00 - a11 - a22);
      a = (a21 - a12 ) / s;
      b = 0.25 * s;
      c = (a01 + a10 ) / s;
      d = (a02 + a20 ) / s;
    }
    else if (a11 > a22)
    {
      double s = 2.0 * sqrt( 1.0 + a11 - a00 - a22);
      a = (a02 - a20 ) / s;
      b = (a01 + a10 ) / s;
      c = 0.25 * s;
      d = (a12 + a21 ) / s;
    }
    else
    {
      double s = 2.0 * sqrt( 1.0 + a22 - a00 - a11 );
      a = (a10 - a01 ) / s;
      b = (a02 + a20 ) / s;
      c = (a12 + a21 ) / s;
      d = 0.25 * s;
    }
  }
  normed = true;
  return *this;
}


inline std::vector <double> Quaternion::rotate(double v1 , double v2, double v3) const
// computes q v q^-1
// for many rotations better convert to rot-matrix and multiply
{
  std::vector <double> vrot(3);
  Quaternion v(0,v1,v2,v3);
  Quaternion p  = getNormalized();
  Quaternion pi = p.getConjugate(); // same as inverse as p is normed
  Quaternion res = p*v*pi;
  vrot[0] = res.b;
  vrot[1] = res.c;
  vrot[2] = res.d;
  return vrot;
}

inline double Quaternion::norm() const
{
  if (normed) return 1.0;
  return sqrt(a*a+b*b+c*c+d*d);
}

inline double Quaternion::norm2() const
{
  if (normed) return 1.0;
  return (a*a+b*b+c*c+d*d);
}

inline Quaternion& Quaternion::normalize()
{
  if (normed) return *this;
  double l = sqrt(a*a+b*b+c*c+d*d);
  this->operator*=(1.0/l);
  normed = true;
  return *this;
}

inline Quaternion Quaternion::getNormalized() const
{
  if (normed) return *this;
  double l = sqrt(a*a+b*b+c*c+d*d);
  return Quaternion(a/l, b/l, c/l, d/l);
}

inline Quaternion Quaternion::getConjugate() const
{
  return Quaternion(a,-b,-c,-d, normed);
}

inline Quaternion& Quaternion::conjugate()
{
  b *= -1;
  c *= -1;
  d *= -1;
  return *this;
}

inline Quaternion Quaternion::getInverse() const
{
  Quaternion qc(a,-b,-c,-d,normed);
  if (normed) return qc;
  double d2 = (a*a+b*b+c*c+d*d);
  return (qc * (1.0/d2));
}

inline Quaternion& Quaternion::invert()
{
  this->conjugate();
  if (normed) return *this;

  double d2 = (a*a+b*b+c*c+d*d);
  this->operator*=(1.0/d2);
  return *this;
}

inline Quaternion& Quaternion::operator=(const Quaternion &vect)
{
  a=vect.a;
  b=vect.b;
  c=vect.c;
  d=vect.d;
  normed = vect.normed;
  return *this;
}


inline int Quaternion::operator==(const Quaternion &vect) const
{
  return ((a==vect.a && b==vect.b && c==vect.c && d==vect.d) ? 1 : 0);
}


inline int Quaternion::operator!=(const Quaternion &vect) const
{
  return ((a==vect.a && b==vect.b && c==vect.c && d==vect.d) ? 0 : 1);
}


// usually the quaternion will not be normed after these operations
// therefore set normed to false;

inline Quaternion& Quaternion::operator+=(const Quaternion &vect)
{
  a+=vect.a;
  b+=vect.b;
  c+=vect.c;
  d+=vect.d;
  normed = false;
  return *this;
}


inline Quaternion& Quaternion::operator-=(const Quaternion &vect)
{
  a-=vect.a;
  b-=vect.b;
  c-=vect.c;
  d-=vect.d;
  normed = false;

  return *this;
}


inline Quaternion& Quaternion::operator*=(const double& scalar)
{
  a*=scalar;
  b*=scalar;
  c*=scalar;
  d*=scalar;
  normed = false;

  return *this;
}


// component wise addition
inline Quaternion Quaternion::operator+(const Quaternion &vect) const
{
  return Quaternion(a+vect.a, b+vect.b, c+vect.c, d+vect.d,false);
}

// component wise subtraction
inline Quaternion Quaternion::operator-(const Quaternion &vect) const
{
  return Quaternion(a-vect.a,b-vect.b, c-vect.c, d-vect.d,false);
}

// Mult Quaternion Quaternion (the order is important!)
inline Quaternion Quaternion::operator*(const Quaternion &vect) const
{
  double an = a*vect.a - b*vect.b - c*vect.c - d*vect.d;
  double bn = a*vect.b + b*vect.a + c*vect.d - d*vect.c;
  double cn = a*vect.c - b*vect.d + c*vect.a + d*vect.b;
  double dn = a*vect.d + b*vect.c - c*vect.b + d*vect.a;
  return Quaternion(an,bn,cn,dn,normed&&vect.normed);
}


// Mult Quaternion scalar
inline Quaternion Quaternion::operator*(const double& scalar) const
{
  return Quaternion(a*scalar, b*scalar, c*scalar, d*scalar,false);
}


// ---------------------------------------------------------- global functions


// Multiplication of scalar and Quaternion from left.
inline Quaternion operator*(const double& scalar, const Quaternion& vect)
{
  return vect*scalar;
}

std::ostream& operator<<(std::ostream& os, const Quaternion& q)
{
  q.write(os);
  return os;
}

//std::istream& operator>>(std::istream& is, Quaternion& q);


#endif
