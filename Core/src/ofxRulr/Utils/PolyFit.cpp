#include "pch_RulrCore.h"
#include "PolyFit.h"

//from http://vilipetek.com/2013/10/17/polynomial-fitting-in-c-not-using-boost/

namespace mathalgo
{
	using namespace std;

	template<class T>
	class matrix
	{
	public:
		matrix(unsigned int nRows, unsigned int nCols) :
			m_nRows(nRows),
			m_nCols(nCols),
			m_oData(nRows*nCols, 0)
		{
			if (!nRows || !nCols)
			{
				throw range_error("invalid matrix size");
			}
		}

		static matrix identity(unsigned int nSize)
		{
			matrix oResult(nSize, nSize);

			int nCount = 0;
			std::generate(oResult.m_oData.begin(), oResult.m_oData.end(),
				[&nCount, nSize]() { return !(nCount++ % (nSize + 1)); });

			return oResult;
		}

		inline T& operator()(unsigned int nRow, unsigned int nCol)
		{
			if (nRow >= m_nRows || nCol >= m_nCols)
			{
				throw out_of_range("position out of range");
			}

			return m_oData[nCol + m_nCols*nRow];
		}

		inline matrix operator*(matrix& other)
		{
			if (m_nCols != other.m_nRows)
			{
				throw domain_error("matrix dimensions are not multiplicable");
			}

			matrix oResult(m_nRows, other.m_nCols);
			for (unsigned int r = 0; r < m_nRows; ++r)
			{
				for (unsigned int ocol = 0; ocol < other.m_nCols; ++ocol)
				{
					for (unsigned int c = 0; c < m_nCols; ++c)
					{
						oResult(r, ocol) += (*this)(r, c) * other(c, ocol);
					}
				}
			}

			return oResult;
		}

		inline matrix transpose()
		{
			matrix oResult(m_nCols, m_nRows);
			for (unsigned int r = 0; r < m_nRows; ++r)
			{
				for (unsigned int c = 0; c < m_nCols; ++c)
				{
					oResult(c, r) += (*this)(r, c);
				}
			}
			return oResult;
		}

		inline unsigned int rows()
		{
			return m_nRows;
		}

		inline unsigned int cols()
		{
			return m_nCols;
		}

		inline vector<T> data()
		{
			return m_oData;
		}

		void print()
		{
			for (unsigned int r = 0; r < m_nRows; r++)
			{
				for (unsigned int c = 0; c < m_nCols; c++)
				{
					std::cout << (*this)(r, c) << "\t";
				}
				std::cout << std::endl;
			}
		}

	private:
		std::vector<T> m_oData;

		unsigned int m_nRows;
		unsigned int m_nCols;
	};

	template<typename T>
	class Givens
	{
	public:
		Givens() : m_oJ(2, 2), m_oQ(1, 1), m_oR(1, 1)
		{
		}

		/*
		Calculate the inverse of a matrix using the QR decomposition.

		param:
		A	matrix to inverse
		*/
		const matrix<T> Inverse(matrix<T>& oMatrix)
		{
			if (oMatrix.cols() != oMatrix.rows())
			{
				throw domain_error("matrix has to be square");
			}
			matrix<T> oIdentity = matrix<T>::identity(oMatrix.rows());
			Decompose(oMatrix);
			return Solve(oIdentity);
		}

		/*
		Performs QR factorization using Givens rotations.
		*/
		void Decompose(matrix<T>& oMatrix)
		{
			int nRows = oMatrix.rows();
			int nCols = oMatrix.cols();


			if (nRows == nCols)
			{
				nCols--;
			}
			else if (nRows < nCols)
			{
				nCols = nRows - 1;
			}

			m_oQ = matrix<T>::identity(nRows);
			m_oR = oMatrix;

			for (int j = 0; j < nCols; j++)
			{
				for (int i = j + 1; i < nRows; i++)
				{
					GivensRotation(m_oR(j, j), m_oR(i, j));
					PreMultiplyGivens(m_oR, j, i);
					PreMultiplyGivens(m_oQ, j, i);
				}
			}

			m_oQ = m_oQ.transpose();
		}

		/*
		Find the solution for a matrix.
		http://en.wikipedia.org/wiki/QR_decomposition#Using_for_solution_to_linear_inverse_problems
		*/
		matrix<T> Solve(matrix<T>& oMatrix)
		{
			matrix<T> oQtM(m_oQ.transpose() * oMatrix);
			int nCols = m_oR.cols();
			matrix<T> oS(1, nCols);
			for (int i = nCols - 1; i >= 0; i--)
			{
				oS(0, i) = oQtM(i, 0);
				for (int j = i + 1; j < nCols; j++)
				{
					oS(0, i) -= oS(0, j) * m_oR(i, j);
				}
				oS(0, i) /= m_oR(i, i);
			}

			return oS;
		}

		const matrix<T>& GetQ()
		{
			return m_oQ;
		}

		const matrix<T>& GetR()
		{
			return m_oR;
		}

	private:
		/*
		Givens rotation is a rotation in the plane spanned by two coordinates axes.
		http://en.wikipedia.org/wiki/Givens_rotation
		*/
		void GivensRotation(T a, T b)
		{
			T t, s, c;
			if (b == 0)
			{
				c = (a >= 0) ? 1 : -1;
				s = 0;
			}
			else if (a == 0)
			{
				c = 0;
				s = (b >= 0) ? -1 : 1;
			}
			else if (abs(b) > abs(a))
			{
				t = a / b;
				s = -1 / sqrt(1 + t*t);
				c = -s*t;
			}
			else
			{
				t = b / a;
				c = 1 / sqrt(1 + t*t);
				s = -c*t;
			}
			m_oJ(0, 0) = c; m_oJ(0, 1) = -s;
			m_oJ(1, 0) = s; m_oJ(1, 1) = c;
		}

		/*
		Get the premultiplication of a given matrix
		by the Givens rotation.
		*/
		void PreMultiplyGivens(matrix<T>& oMatrix, int i, int j)
		{
			int nRowSize = oMatrix.cols();

			for (int nRow = 0; nRow < nRowSize; nRow++)
			{
				double nTemp = oMatrix(i, nRow) * m_oJ(0, 0) + oMatrix(j, nRow) * m_oJ(0, 1);
				oMatrix(j, nRow) = oMatrix(i, nRow) * m_oJ(1, 0) + oMatrix(j, nRow) * m_oJ(1, 1);
				oMatrix(i, nRow) = nTemp;
			}
		}

	private:
		matrix<T> m_oQ, m_oR, m_oJ;
	};

	/*
	Finds the coefficients of a polynomial p(x) of degree n that fits the data,
	p(x(i)) to y(i), in a least squares sense. The result p is a row vector of
	length n+1 containing the polynomial coefficients in incremental powers.

	param:
	oX				x axis values
	oY				y axis values
	nDegree			polynomial degree including the constant

	return:
	coefficients of a polynomial starting at the constant coefficient and
	ending with the coefficient of power to nDegree. C++0x-compatible
	compilers make returning locally created vectors very efficient.

	*/
	template<typename T>
	std::vector<T> polyfit(const std::vector<T>& oX, const std::vector<T>& oY, int nDegree)
	{
		if (oX.size() != oY.size())
			throw std::invalid_argument("X and Y vector sizes do not match");

		// more intuative this way
		nDegree++;

		size_t nCount = oX.size();
		matrix<T> oXMatrix(nCount, nDegree);
		matrix<T> oYMatrix(nCount, 1);

		// copy y matrix
		for (size_t i = 0; i < nCount; i++)
		{
			oYMatrix(i, 0) = oY[i];
		}

		// create the X matrix
		for (size_t nRow = 0; nRow < nCount; nRow++)
		{
			T nVal = 1.0f;
			for (int nCol = 0; nCol < nDegree; nCol++)
			{
				oXMatrix(nRow, nCol) = nVal;
				nVal *= oX[nRow];
			}
		}

		// transpose X matrix
		matrix<T> oXtMatrix(oXMatrix.transpose());
		// multiply transposed X matrix with X matrix
		matrix<T> oXtXMatrix(oXtMatrix * oXMatrix);
		// multiply transposed X matrix with Y matrix
		matrix<T> oXtYMatrix(oXtMatrix * oYMatrix);

		Givens<T> oGivens;
		oGivens.Decompose(oXtXMatrix);
		matrix<T> oCoeff = oGivens.Solve(oXtYMatrix);
		// copy the result to coeff
		return oCoeff.data();
	}

	/*
	Calculates the value of a polynomial of degree n evaluated at x. The input
	argument pCoeff is a vector of length n+1 whose elements are the coefficients
	in incremental powers of the polynomial to be evaluated.

	param:
	oCoeff			polynomial coefficients generated by polyfit() function
	oX				x axis values

	return:
	Fitted Y values. C++0x-compatible compilers make returning locally
	created vectors very efficient.
	*/
	template<typename T>
	std::vector<T> polyval(const std::vector<T>& oCoeff, const std::vector<T>& oX)
	{
		size_t nCount = oX.size();
		size_t nDegree = oCoeff.size();
		std::vector<T>	oY(nCount);

		for (size_t i = 0; i < nCount; i++)
		{
			T nY = 0;
			T nXT = 1;
			T nX = oX[i];
			for (size_t j = 0; j < nDegree; j++)
			{
				// multiply current x by a coefficient
				nY += oCoeff[j] * nXT;
				// power up the X
				nXT *= nX;
			}
			oY[i] = nY;
		}

		return oY;
	}
}


namespace ofxRulr {
	namespace Utils {
		namespace PolyFit {
			Model fit(const vector<ofVec2f> & data, int order) {
				vector<float> x(data.size()), y(data.size());
				for (int i = 0; i < data.size(); i++) {
					x[i] = data[i].x;
					y[i] = data[i].y;
				}

				return Model({mathalgo::polyfit<float>(x, y, order)});
			}

			float evaluate(const Model & model, float x) {
				return mathalgo::polyval<float>(model.parameters, vector<float>(1, x))[0];
			}
		}
	}
}

