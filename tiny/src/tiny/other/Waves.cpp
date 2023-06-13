#include "tiny-pch.h"
#include "Waves.h"
#include "tiny/utils/Profile.h"

using namespace DirectX;

namespace tiny
{
Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
{
	mNumRows = m;
	mNumCols = n;

	mVertexCount = m * n;
	mTriangleCount = (m - 1) * (n - 1) * 2;

	mTimeStep = dt;
	mSpatialStep = dx;

	float d = damping * dt + 2.0f;
	float e = (speed * speed) * (dt * dt) / (dx * dx);
	mK1 = (damping * dt - 2.0f) / d;
	mK2 = (4.0f - 8.0f * e) / d;
	mK3 = (2.0f * e) / d;

	mPrevSolution.resize(static_cast<size_t>(m) * n);
	mCurrSolution.resize(static_cast<size_t>(m) * n);
	mNormals.resize(static_cast<size_t>(m) * n);
	mTangentX.resize(static_cast<size_t>(m) * n);

	// Generate grid vertices in system memory.

	float halfWidth = (n - 1) * dx * 0.5f;
	float halfDepth = (m - 1) * dx * 0.5f;
	for (int i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dx;
		for (int j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			mPrevSolution[static_cast<size_t>(i) * n + j] = XMFLOAT3(x, 0.0f, z);
			mCurrSolution[static_cast<size_t>(i) * n + j] = XMFLOAT3(x, 0.0f, z);
			mNormals[static_cast<size_t>(i) * n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
			mTangentX[static_cast<size_t>(i) * n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
		}
	}
}

Waves::~Waves()
{
}

int Waves::RowCount()const
{
	return mNumRows;
}

int Waves::ColumnCount()const
{
	return mNumCols;
}

int Waves::VertexCount()const
{
	return mVertexCount;
}

int Waves::TriangleCount()const
{
	return mTriangleCount;
}

float Waves::Width()const
{
	return mNumCols * mSpatialStep;
}

float Waves::Depth()const
{
	return mNumRows * mSpatialStep;
}

void Waves::Update(float dt)
{
	PROFILE_FUNCTION();

	static float t = 0;

	// Accumulate time.
	t += dt;

	// Only update the simulation at the specified time step.
	if (t >= mTimeStep)
	{
		// Only update interior points; we use zero boundary conditions.
		concurrency::parallel_for(1, mNumRows - 1, [this](int i)
			//for(int i = 1; i < mNumRows-1; ++i)
			{
				for (int j = 1; j < mNumCols - 1; ++j)
				{
					// After this update we will be discarding the old previous
					// buffer, so overwrite that buffer with the new update.
					// Note how we can do this inplace (read/write to same element) 
					// because we won't need prev_ij again and the assignment happens last.

					// Note j indexes x and i indexes z: h(x_j, z_i, t_k)
					// Moreover, our +z axis goes "down"; this is just to 
					// keep consistent with our row indices going down.

					mPrevSolution[static_cast<size_t>(i) * mNumCols + j].y =
						mK1 * mPrevSolution[static_cast<size_t>(i) * mNumCols + j].y +
						mK2 * mCurrSolution[static_cast<size_t>(i) * mNumCols + j].y +
						mK3 * (mCurrSolution[(static_cast<size_t>(i) + 1) * mNumCols + j].y +
							mCurrSolution[(static_cast<size_t>(i) - 1) * mNumCols + j].y +
							mCurrSolution[static_cast<size_t>(i) * mNumCols + j + 1].y +
							mCurrSolution[static_cast<size_t>(i) * mNumCols + j - 1].y);
				}
			});

		// We just overwrote the previous buffer with the new data, so
		// this data needs to become the current solution and the old
		// current solution becomes the new previous solution.
		std::swap(mPrevSolution, mCurrSolution);

		t = 0.0f; // reset time

		//
		// Compute normals using finite difference scheme.
		//
		concurrency::parallel_for(1, mNumRows - 1, [this](int i)
			//for(int i = 1; i < mNumRows - 1; ++i)
			{
				for (int j = 1; j < mNumCols - 1; ++j)
				{
					float l = mCurrSolution[static_cast<size_t>(i) * mNumCols + j - 1].y;
					float r = mCurrSolution[static_cast<size_t>(i) * mNumCols + j + 1].y;
					float t = mCurrSolution[(static_cast<size_t>(i) - 1) * mNumCols + j].y;
					float b = mCurrSolution[(static_cast<size_t>(i) + 1) * mNumCols + j].y;
					mNormals[static_cast<size_t>(i) * mNumCols + j].x = -r + l;
					mNormals[static_cast<size_t>(i) * mNumCols + j].y = 2.0f * mSpatialStep;
					mNormals[static_cast<size_t>(i) * mNumCols + j].z = b - t;

					XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&mNormals[static_cast<size_t>(i) * mNumCols + j]));
					XMStoreFloat3(&mNormals[static_cast<size_t>(i) * mNumCols + j], n);

					mTangentX[static_cast<size_t>(i) * mNumCols + j] = XMFLOAT3(2.0f * mSpatialStep, r - l, 0.0f);
					XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&mTangentX[static_cast<size_t>(i) * mNumCols + j]));
					XMStoreFloat3(&mTangentX[static_cast<size_t>(i) * mNumCols + j], T);
				}
			});
	}
}

void Waves::Disturb(int i, int j, float magnitude)
{
	// Don't disturb boundaries.
	assert(i > 1 && i < mNumRows - 2);
	assert(j > 1 && j < mNumCols - 2);

	float halfMag = 0.5f * magnitude;

	// Disturb the ijth vertex height and its neighbors.
	mCurrSolution[static_cast<size_t>(i) * mNumCols + j].y += magnitude;
	mCurrSolution[static_cast<size_t>(i) * mNumCols + j + 1].y += halfMag;
	mCurrSolution[static_cast<size_t>(i) * mNumCols + j - 1].y += halfMag;
	mCurrSolution[(static_cast<size_t>(i) + 1) * mNumCols + j].y += halfMag;
	mCurrSolution[(static_cast<size_t>(i) - 1) * mNumCols + j].y += halfMag;
}

}