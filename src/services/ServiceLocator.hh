#ifndef RA_SERVICE_LOCATOR_HH
#define RA_SERVICE_LOCATOR_HH
#pragma once

#include <assert.h>
#include <memory>

namespace ra {
namespace services {

class ServiceLocator
{
public:
    /* returns a pointer to the requested service, or nullptr if not found
     */
    template <class TClass>
    static const TClass* Get()
    {
        return Service<TClass>::Get();
    }

    /* returns a pointer to the requested service, or nullptr if not found
     *
     * WARNING: mutable services are not guaranteed to be thread-safe. prefer using non-mutable
     *          services whenever possible.
     */
    template <class TClass>
    static TClass* GetMutable()
    {
        return Service<TClass>::Get();
    }
    
    /* registers a service
     */
    template <class TClass>
    static void Provide(TClass* pInstance)
    {
        Service<TClass>::s_pInstance.reset(pInstance);
    }
    
    /* temporarily overrides a service registration - primarily used in unit tests
     */
    template <class TClass>
    class ServiceOverride
    {
    public:
        /*  provides a temporary implementation for a service
         *
         *  @param pOverride - the temporary implementation
         *  @param bDestroy  - true to delete the object when the ServiceOverride goes out of scope.
         *                     typically false as the temporary implementation is normally a 'this' pointer 
         *                     or a stack variable
         */
        ServiceOverride(TClass* pOverride, bool bDestroy = false)
        {
            m_pPrevious = Service<TClass>::s_pInstance.release();
            Service<TClass>::s_pInstance.reset(pOverride);
            
            m_bDestroy = bDestroy;
        }
        
        ~ServiceOverride()
        {
            if (!m_bDestroy)
                Service<TClass>::s_pInstance.release();

            Service<TClass>::s_pInstance.reset(m_pPrevious);
        }
        
    private:
        TClass* m_pPrevious;
        bool m_bDestroy;
    };
    
private:
    template <class TClass>
    class Service
    {
    public:
        static TClass* Get() 
        {
            assert(s_pInstance != nullptr);
            return s_pInstance.get();
        }
        
        static std::unique_ptr<TClass> s_pInstance;
    };
};

template <class TClass>
std::unique_ptr<TClass> ServiceLocator::Service<TClass>::s_pInstance;

} // namespace services
} // namespace ra

#endif // !RA_SERVICE_LOCATOR_HH
