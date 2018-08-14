
//
// 2018 (c) Nanami Yamamoto
// This class is used to treat the decimal as a fraction.
//



#ifndef _FRACTION_HPP_
#define _FRACTION_HPP_


namespace math {

	struct frac_t {
		int n, d;
	};


	struct Fraction {

	public:
		Fraction();
		Fraction(int n, int d = 1);
		~Fraction();

		Fraction &set(int n, int d = 1);
		frac_t get() const { return { numer, denom }; }

		float to_f() { return static_cast<float>(numer)/denom; }
		int to_i() { return numer/denom; }

		void print();

		Fraction &reduce();

		Fraction operator+() const;
		Fraction operator-() const;
		Fraction &operator=(int R);
		Fraction &operator+=(const Fraction &R);
		Fraction &operator+=(int R);
		Fraction &operator-=(const Fraction &R);
		Fraction &operator-=(int R);
		Fraction &operator*=(const Fraction &R);
		Fraction &operator*=(int R);
		Fraction &operator/=(const Fraction &R);
		Fraction &operator/=(int R);


	private:
		int numer;
		int denom;
	};


	int gcd(int a, int b);
	int lcm(int a, int b);

	void adjustDenom(Fraction &a, Fraction &b);


	template<class T1, class T2>
	bool operator==(const T1 &L, const T2 &R) {

		Fraction fracL(L);
		Fraction fracR(R);

		if (fracL.get().n == 0 && fracR.get().n == 0)
			return true;

		adjustDenom(fracL, fracR);

		return (fracL.get().n == fracR.get().n);
	}

	template<class T1, class T2>
	bool operator!=(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		return !(fracL == fracR);
	}

	template<class T1, class T2>
	bool operator<(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		adjustDenom(fracL, fracR);

		return (fracL.get().n < fracR.get().n);
	}


	template<class T1, class T2>
	bool operator>(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		return fracR < fracL;
	}

	
	template<class T1, class T2>
	bool operator<=(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		return !(fracL > fracR);
	}

	template<class T1, class T2>
	bool operator>=(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		return !(fracL < fracR);
	}


	template<class T1, class T2>
	Fraction operator+(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		adjustDenom(fracL, fracR);
		int ans_numer = fracL.get().n + fracR.get().n;

		return Fraction(ans_numer, fracL.get().d);
	}

	template<class T1, class T2>
	Fraction operator-(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		adjustDenom(fracL, fracR);
		int ans_numer = fracL.get().n - fracR.get().n;

		return Fraction(ans_numer, fracL.get().d);
	}


	template<class T1, class T2>
	Fraction operator*(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		int n = fracL.get().n * fracR.get().n;
		int d = fracL.get().d * fracR.get().d;

		return Fraction(n, d);
	}

	template<class T1, class T2>
	Fraction operator/(const T1 &L, const T2 &R) {
		Fraction fracL(L);
		Fraction fracR(R);

		try {

			if (fracR == 0)
				throw std::runtime_error("[class:Fraction] divide by zero");

		} catch (const std::exception& e) {
			std::cout << e.what();
			throw;
		}

		int n = fracL.get().n * fracR.get().d;
		int d = fracL.get().d * fracR.get().n;

		return Fraction(n, d);
	}

}


#endif // !_FRACTION_HPP_

