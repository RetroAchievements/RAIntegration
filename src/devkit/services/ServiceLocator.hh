#ifndef RA_SERVICE_LOCATOR_HH
#define RA_SERVICE_LOCATOR_HH
#pragma once

#include <functional>
#include <memory>
#include <string>

#include <gsl/assert>

namespace ra {
namespace services {

class ServiceLocator
{
public:
    /// <summary>
    /// Returns a reference to a service implementing the requested interface.
    /// </summary>
    /// <returns>Reference to an implementation of the interface</returns>
    template <class TClass>
    static const TClass& Get() noexcept
    {
        return Service<TClass>::Get();
    }

    /// <summary>
    /// Returns a non-const reference to a service implementing the requested interface.
    /// </summary>
    /// <returns>Reference to an implementation of the interface</returns>
    /// <remarks>
    /// WARNING: mutable services are not guaranteed to be thread-safe. Prefer using non-mutable 
    /// services whenever possible.
    /// </remarks>
    template <class TClass>
    static TClass& GetMutable() noexcept
    {
        return Service<TClass>::Get();
    }

    /// <summary>
    /// Determines whether or not a service implementing the requested interface was registered.
    /// </summary>
    /// <returns><c>true</c> if an implementation exists, <c>false</c> if not.</returns>
    template <class TClass>
    static bool Exists() noexcept
    {
        return Service<TClass>::Exists();
    }

    /// <summary>
    /// Registers a service implementation for an interface
    /// </summary>
    /// <param name="pInstance">
    /// A pointer to an allocated instance of a service implementation. ServiceLocator will own the 
    /// pointer and delete it at the end of execution or when a different implementation is provided.
    /// </param>
    template <class TClass>
    static void Provide(std::unique_ptr<TClass> pInstance) noexcept
    {
        Service<TClass>::s_pProvidedInstance = std::move(pInstance);
        Service<TClass>::s_pCurrentInstance = Service<TClass>::s_pProvidedInstance.get();
    }

    /// <summary>
    /// Gets whether or not services have been initialized
    /// </summary>
    static bool IsInitialized() noexcept;

    /// <summary>
    /// Gets whether or not shutdown has started
    /// </summary>
    static bool IsShuttingDown() noexcept;

    /// <summary>
    /// Provides a callback to run when a service is not found.
    /// </summary>
    /// <remarks>
    /// Used by the unit test framework to assert failure instead of terminating the test runner.
    /// </remarks>
    static void SetNoServiceHandler(std::function<void(const std::string&)> fHandler);

    /// <summary>
    /// Provides a temporary implementation of an interface for the duration of the scope of the ServiceOverride.
    /// The original implementation will be restored when the <see cref="ServiceOverride"/> goes out of scope.
    /// </summary>
    /// <remarks>
    /// Intended for use only in unit tests.
    /// </remarks>
    template <class TClass>
    class ServiceOverride
    {
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="ServiceOverride" /> class.
        /// </summary>
        /// <param name="pOverride">A pointer to the temporary implementation.</param>
        explicit ServiceOverride(TClass* pOverride) noexcept
            : m_pPreviousInstance(Service<TClass>::s_pCurrentInstance)
        {
            Service<TClass>::s_pCurrentInstance = pOverride;
        }

        ServiceOverride() = delete;

        ~ServiceOverride() noexcept
        {
            Service<TClass>::s_pCurrentInstance = m_pPreviousInstance;
        }

        ServiceOverride(const ServiceOverride&) noexcept = delete;
        ServiceOverride& operator=(const ServiceOverride&) noexcept = delete;
        ServiceOverride(ServiceOverride&&) noexcept = delete;
        ServiceOverride& operator=(ServiceOverride&&) noexcept = delete;

    private:
        TClass* m_pPreviousInstance;
    };

private:
    static void ReportNoServiceProvided(const char* sFunctionName) noexcept;

    template <class TClass>
    class Service
    {
    public:
        static TClass& Get() noexcept
        {
            if (GSL_UNLIKELY(s_pCurrentInstance == nullptr))
                ReportNoServiceProvided();

            return *s_pCurrentInstance;
        }

        static bool Exists() noexcept
        {
            return (s_pCurrentInstance != nullptr);
        }

        static std::unique_ptr<TClass> s_pProvidedInstance;
        static TClass* s_pCurrentInstance;

    private:
        static void ReportNoServiceProvided() noexcept
        {
#if defined(__PRETTY_FUNCTION__)
            const char* sFunctionName = __PRETTY_FUNCTION__;
#elif defined (__FUNCTION__)
            const char* sFunctionName = __FUNCTION__;
#else
            const char* sFunctionName = __FUNC__;
#endif
            ServiceLocator::ReportNoServiceProvided(sFunctionName);
        }
    };
};

template <class TClass>
std::unique_ptr<TClass> ServiceLocator::Service<TClass>::s_pProvidedInstance;

template<class TClass>
TClass* ServiceLocator::Service<TClass>::s_pCurrentInstance = nullptr;

} // namespace services
} // namespace ra

#endif // !RA_SERVICE_LOCATOR_HH
