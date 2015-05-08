
#include "arch/Sextets/Threads/InterlockedPointerVariable.h"
#include "arch/Sextets/Threads/MutexNames.h"

#include "RageThreads.h"

#include <sstream>

namespace Sextets
{
	namespace Threads
	{
		class _InterlockedUntypedPointerVariable::_Impl :
			public _InterlockedUntypedPointerVariable
		{
			private:
				void* value;
				RageMutex mutex;

				inline void lock() { mutex.Lock(); }
				inline void unlock() { mutex.Unlock(); }

				inline bool rawCompareAndSet(void* expected, void* replacement)
				{
					if(expected == value) {
						value = replacement;
						return true;
					}
					return false;
				}

				inline void* rawGet()
				{
					return value;
				}

				inline void* rawGetAndSet(void* replacement)
				{
					void* old = value;
					value = replacement;
					return old;
				}

				inline void rawSet(void* replacement)
				{
					value = replacement;
				}

			public:
				_Impl(void * initialValue) :
					value(initialValue),
					mutex(MutexNames::Make("_InterlockedUntypedPointerVariable::_Impl", this))
				{
				}

				virtual ~_Impl()
				{
				}

				virtual bool CompareAndSet(void* expected, void* replacement)
				{
					lock();
					bool result = rawCompareAndSet(expected, replacement);
					unlock();
					return result;
				}

				virtual void* Get()
				{
					lock();
					void* result = rawGet();
					unlock();
					return result;
				}

				virtual void* GetAndSet(void* replacement)
				{
					lock();
					void* result = rawGetAndSet(replacement);
					unlock();
					return result;
				}

				virtual void Set(void* replacement)
				{
					lock();
					rawSet(replacement);
					unlock();
				}
		};

		_InterlockedUntypedPointerVariable* _InterlockedUntypedPointerVariable::Create(void* initialValue)
		{
			return new _Impl(initialValue);
		}
	}
}

