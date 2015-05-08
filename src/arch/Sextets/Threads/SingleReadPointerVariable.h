
#ifndef Sextets_Threads_SingleReadPointerVariable_h
#define Sextets_Threads_SingleReadPointerVariable_h

#include "arch/Sextets/Threads/InterlockedPointerVariable.h"

namespace Sextets
{
	namespace Threads
	{
		/** @brief A pointer container that returns its initial value to the
		 * first call to `Claim()`, then NULL to all subsequent calls.
		 * 
		 * Calls to Claim() are thread-safe; a maximum of one caller will
		 * actually receive the initial value even if multiple threads call
		 * simultaneously.
		 */
		template <typename T> class SingleReadPointerVariable
		{
			private:
				volatile bool _invalidated;
				InterlockedPointerVariable<T> _v;
				bool _deleteIfUnclaimed;
			public:
				/** @brief Constructs a SingleReadPointerVariable with the
				 * given initial value.
				 *
				 * @param value The initial value.
				 * @param deleteIfUnclaimed	If true, and if the initial
				 *		value unclaimed and non-NULL while this container is
				 *		being destroyed, the value will be deleted at that
				 *		time. If false, this container will never delete the
				 *		value.
				 */
				SingleReadPointerVariable(T* value, bool deleteIfUnclaimed = true) :
					_invalidated(false),
					_v(value),
					_deleteIfUnclaimed(deleteIfUnclaimed)
				{
				}

				virtual ~SingleReadPointerVariable()
				{
					if(_deleteIfUnclaimed) {
						T* p = Claim();
						if(p != NULL) {
							delete p;
						}
					}
				}

				/** @brief Returns the initial value on the first call or
				 * NULL on subsequent calls.
				 *
				 * If a non-NULL value is returned from here and
				 * `deleteIfUnclaimed` was set to true, in general the
				 * caller is responsible for deleting it, though this is not
				 * quite a rule.
				 */
				virtual T* Claim()
				{
					if(_invalidated) {
						return NULL;
					}
					else {
						_invalidated = true;
						return _v.GetAndSet(NULL);
					}
				}
		};
	}
}


#endif
//KWH
