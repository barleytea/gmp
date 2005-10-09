

#include "bigmod.h"

std::string bigmod::str(int b) const
{
  if (value.isNA())
    return "NA";

  std::string s; // sstream seems to collide with libgmp :-(
  if (!modulus.isNA())
    s = "(";
  s += value.str(b);
  if (!modulus.isNA()) {
    s += " %% ";
    s += modulus.str(b);
    s += ")";
  }
  return s;
}

bigmod & bigmod::operator= (const bigmod& rhs)
{
  if(this != &rhs)
    {
      modulus = rhs.modulus;
      value = rhs.value;
    }
  return(*this);
}

bigmod bigmod::inv () const
{
  if(value.isNA() || modulus.isNA())
    return(bigmod());

  mpz_t val;
  mpz_init(val);
  mpz_t_sentry val_s(val);
  if (mpz_invert(val, value.getValueTemp(), modulus.getValueTemp()) == 0)
    Rf_error("argument has no inverse");
  return bigmod(val, modulus );

}


bool operator!=(const bigmod& rhs, const bigmod& lhs)
{
  if(rhs.value != lhs.value)
    return(true);
  return(rhs.modulus != lhs.modulus);
}

bool operator==(const bigmod& rhs, const bigmod& lhs)
{
  if(rhs.value != lhs.value)
    return(false);
  return(!(rhs.modulus != lhs.modulus));
}


bigmod operator+(const bigmod& lhs, const bigmod& rhs)
{
  return create_bigmod(lhs, rhs, mpz_add);
}

bigmod operator-(const bigmod& lhs, const bigmod& rhs)
{
  return create_bigmod(lhs, rhs, mpz_sub);
}

bigmod operator*(const bigmod& lhs, const bigmod& rhs)
{
  return create_bigmod(lhs, rhs, mpz_mul);
}

bigmod operator/(const bigmod& lhs, const bigmod& rhs)
{
  return create_bigmod(lhs, rhs, mpz_tdiv_q, false);
}

bigmod operator%(const bigmod& lhs, const bigmod& rhs)
{
  if (lhs.value.isNA() || rhs.value.isNA())
    return bigmod();
  if (mpz_sgn(rhs.value.getValueTemp()) == 0)
    Rf_error("division by zero");
  biginteger mod;
  if (!lhs.modulus.isNA() || !rhs.modulus.isNA())
    mod = rhs.value;

  mpz_t val;
  mpz_init(val);
  mpz_t_sentry val_s(val);
  mpz_mod(val, lhs.value.getValueTemp(), rhs.value.getValueTemp());
  return bigmod(val, mod);
}



bigmod pow(const bigmod& base, const bigmod& exp)
{
  if (base.value.isNA() || exp.value.isNA())
    return bigmod();
  mpz_t val;
  mpz_init(val);
  mpz_t_sentry val_s(val);
  biginteger mod = get_modulus(base, exp);
  if (mod.isNA()) {
    if(mpz_sgn(exp.value.getValueTemp() ) <0)
      Rf_error("Negative values not allowed");
    if (!mpz_fits_ulong_p(exp.value.getValueTemp()))
      Rf_error("exponent too large for pow");
    mpz_pow_ui(val, base.value.getValueTemp(), mpz_get_ui(exp.value.getValueTemp()));
  } else
    mpz_powm(val, base.value.getValueTemp(), exp.value.getValueTemp(), mod.getValueTemp());
  return bigmod(val, mod);
}

bigmod inv(const bigmod& x, const bigmod& m)
{
  if (x.value.isNA() || m.value.isNA())
    return bigmod();
  if (mpz_sgn(m.value.getValueTemp()) == 0)
    Rf_error("division by zero");
  biginteger mod = get_modulus(x, m);
  mpz_t val;
  mpz_init(val);
  mpz_t_sentry val_s(val);
  if (mpz_invert(val, x.value.getValueTemp(), m.value.getValueTemp()) == 0)
    Rf_error("argument has no inverse");
  return bigmod(val, mod);
}

bigmod set_modulus(const bigmod& x, const bigmod& m)
{

  if (!m.value.isNA() && mpz_sgn(m.value.getValueTemp()) == 0)
    Rf_error("division by zero");
  //    if (!m.value.isNA() && mpz_cmp(x.value.getValueTemp(),m.value.getValueTemp())>=0) {
  if (!m.value.isNA() ) {
    bigmod t(x%m);
    return bigmod(t.value, m.value);
  } else
    return bigmod(x.value, m.value);
}

bigmod gcd(const bigmod& lhs, const bigmod& rhs)
{
  return create_bigmod(lhs, rhs, mpz_gcd);
}

bigmod lcm(const bigmod& lhs, const bigmod& rhs)
{
  return create_bigmod(lhs, rhs, mpz_lcm);
}


// return the modulus to use for this two bigmods.
// NA if incompatible.
biginteger get_modulus(const bigmod& b1, const bigmod& b2)
{
  if (b1.modulus.isNA())
    return b2.modulus; // if b2 is NA too, the return is correct: NA
  else if (b2.modulus.isNA())
    return b1.modulus;	
  else if (mpz_cmp(b1.modulus.getValueTemp(), b2.modulus.getValueTemp()))
    return biginteger(); // TODO: warning possible
  else // equal
    return b1.modulus;
}

    
//    typedef void (*gmp_binary)(mpz_t, const mpz_t, const mpz_t);


 

// Create a bigmod from a binary combination of two other bigmods
bigmod create_bigmod(const bigmod& lhs, const bigmod& rhs, gmp_binary f,
		     bool zeroRhsAllowed) {
  if (lhs.value.isNA() || rhs.value.isNA())
    return bigmod();
  if (!zeroRhsAllowed && mpz_sgn(rhs.value.getValueTemp()) == 0)
    Rf_error("division by zero");
  biginteger mod = get_modulus(lhs, rhs);
  mpz_t val;
  mpz_init(val);
  mpz_t_sentry val_s(val);
  f(val, lhs.value.getValueTemp(), rhs.value.getValueTemp());
  if (!mod.isNA())
    mpz_mod(val, val, mod.getValueTemp());
  return bigmod(val, mod);
}