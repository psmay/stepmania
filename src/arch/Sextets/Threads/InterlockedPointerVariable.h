
#ifndef Sextets_Threads_InterlockedPointerVariable_h
#define Sextets_Threads_InterlockedPointerVariable_h

#include "global.h"

namespace Sextets
{
	namespace Threads
	{
		class _InterlockedUntypedPointerVariable
		{
			private:
				class _Impl;

			protected:
				_InterlockedUntypedPointerVariable() {}

			public:
				virtual ~_InterlockedUntypedPointerVariable() {};
				virtual bool CompareAndSet(void* expected, void* replacement) = 0;
				virtual void* Get() = 0;
				virtual void* GetAndSet(void* replacement) = 0;
				virtual void Set(void* replacement) = 0;

				static _InterlockedUntypedPointerVariable* Create(void* initialValue = NULL);
		};

		template <typename T> class InterlockedPointerVariable
		{
			private:
				_InterlockedUntypedPointerVariable * _vp;
			public:
				InterlockedPointerVariable(T* initialValue) :
					_vp(_InterlockedUntypedPointerVariable::Create(initialValue))
				{
				}

				virtual ~InterlockedPointerVariable()
				{
					delete _vp;
				}

				virtual bool CompareAndSet(T* expected, T* replacement)
				{
					return _vp->CompareAndSet(expected, replacement);
				}

				virtual T* Get()
				{
					return (T*)(_vp->Get());
				}

				virtual T* GetAndSet(T* replacement)
				{
					return (T*)(_vp->GetAndSet(replacement));
				}

				virtual void Set(T* replacement)
				{
					_vp->Set(replacement);
				}
		};
	}
}

#endif

