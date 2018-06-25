#ifndef IRA_WINDOWCLASS_H
#define IRA_WINDOWCLASS_H
#pragma once

#include <WTypes.h>


namespace ra {
	/// <summary>
	///   Extremely simple window class, derive to make specialized versions
	/// </summary>
	/// <seealso cref="WNDCLASSEX" />
	class IRA_WndClass : protected WNDCLASSEX
	{
		friend class MemoryViewerControl;
	public:
		IRA_WndClass(LPCTSTR className) noexcept;
		virtual ~IRA_WndClass() noexcept;
		IRA_WndClass(const IRA_WndClass&) = default;
		IRA_WndClass& operator=(const IRA_WndClass&) = default;
		IRA_WndClass(IRA_WndClass&&) noexcept = default;
		IRA_WndClass& operator=(IRA_WndClass&&) noexcept = default;

		// virtual stuff should be limited
		bool Register() noexcept;
		LPCTSTR className() const noexcept { return lpszClassName; }
	};
} // namespace ra

#endif // !IRA_WINDOWCLASS_H

