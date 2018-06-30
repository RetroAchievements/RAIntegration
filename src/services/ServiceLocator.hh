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
    /* returns a referece to a service implementing the requested interface
     * 
     * @throw std::runtime_error if no service is provided for the requested interface
     */
    template <class TClass>
    static const TClass& Get()
    {
        return Service<TClass>::Get();
    }

    /* returns a referece to a service implementing the requested interface
     *
     * @throw std::runtime_error if no service is provided for the requested interface
     *
     * WARNING: mutable services are not guaranteed to be thread-safe. prefer using non-mutable
     *          services whenever possible.
     */
    template <class TClass>
    static TClass& GetMutable()
    {
        return Service<TClass>::Get();
    }
    
    /* registers a service implementation for an interface
     *
     * @param pInstance    externally allocated instance (via new operator). ServiceLocator will delete at end of execution or when replaced.
     */
    template <class TClass>
    static void Provide(TClass* pInstance)
    {
        Service<TClass>::s_pInstance.reset(pInstance);
    }
    
    /* temporarily overrides a service implementation - primarily used in unit tests
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
        static TClass& Get() 
        {
            if (s_pInstance == nullptr)
                ThrowNoServiceProvidedException();

            return *s_pInstance.get();
        }
        
        static std::unique_ptr<TClass> s_pInstance;

    private:
        static void ThrowNoServiceProvidedException()
        {
#if defined(__PRETTY_FUNCTION__)
            std::string sMessage = __PRETTY_FUNCTION__;
#elif defined (__FUNCTION__)
            std::string sMessage = __FUNCTION__;
#else
            std::string sMessage = __FUNC__;
#endif
            size_t index = sMessage.find('<');
            size_t index2 = sMessage.rfind('>');
            if (index >= 0 && index2 > index)
            {
                sMessage.erase(index2);
                sMessage.erase(0, index + 1);
            }
            sMessage.insert(0, "No service provided for ");

            OutputDebugStringA("ERROR: ");
            OutputDebugStringA(sMessage.c_str());
            OutputDebugStringA("\n");
            throw new std::runtime_error(sMessage);
        }
    };
};

template <class TClass>
std::unique_ptr<TClass> ServiceLocator::Service<TClass>::s_pInstance;

} // namespace services
} // namespace ra

#endif // !RA_SERVICE_LOCATOR_HH
