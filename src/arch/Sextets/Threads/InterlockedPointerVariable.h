
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

		/** @brief A container that implements atomic access to a pointer.
		 *
		 * All methods are implemented in, or as if in, critical sections so
		 * that only one thread may act on the value at a time.
		 */
		template <typename T> class InterlockedPointerVariable
		{
			private:
				_InterlockedUntypedPointerVariable * _vp;
			public:
				/** @brief Constructs a new InterlockedPointerVariable with
				 * the given initial value. */
				InterlockedPointerVariable(T* initialValue) :
					_vp(_InterlockedUntypedPointerVariable::Create(initialValue))
				{
				}

				virtual ~InterlockedPointerVariable()
				{
					delete _vp;
				}

				/** @brief Replaces the value in this container, but only if
				 * the existing value is the expected value. */
				virtual bool CompareAndSet(T* expected, T* replacement)
				{
					return _vp->CompareAndSet(expected, replacement);
				}

				/** @brief Retrieves the value in this container. */
				virtual T* Get()
				{
					return (T*)(_vp->Get());
				}

				/** @brief Replaces the value in this container, then
				 * returns the previous value. */
				virtual T* GetAndSet(T* replacement)
				{
					return (T*)(_vp->GetAndSet(replacement));
				}

				/** @brief Unconditionally replaces the value in this
				 * container. */
				virtual void Set(T* replacement)
				{
					_vp->Set(replacement);
				}
		};
	}
}

#endif
//KWH
