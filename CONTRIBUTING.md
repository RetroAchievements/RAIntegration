# Contributing to RAIntegration

If you are a developer and want to contribute to the development of RAIntegration, please read this.

# Pull Requests
Outside contributions are generally only accepted in the form of a pull request. The process is very simple.
Fork RAIntegration, make your changes, and issue a pull request on GitHub. This can all be done within the browser.
The changes are reviewed, and might be merged in. If the pull request isn't acceptable at the time,
note that it's possible to continue pushing up commits to your branch.

If you want to develop a larger feature, we'd like to discuss this first (ideally on Discord) so that you don't 
risk developing something that won't be merged. A pull request with a proof-of-concept is fine, but please indicate so.

# Coding style
Having a consistent code style throughout the code base is highly valued.
Please look through the code to get a feel for the coding style.
A pull request may be asked to fix the coding style before submission.
In other cases, a pull request may be followed up with a "style nit commit".

### Variables
* Variable names should use [Hungarian Notation](https://en.wikipedia.org/wiki/Hungarian_notation). The name is `PascalCase` with a one or two letter prefix indicating type: 
  * `int nCounter`
  * `std::string sLabel`
  * `bool bCondition`
  * `HWND hWindow`
  * `RECT rcSize`
* Class member variables should follow the above description and be prefixed with `m_`:
  * `bool m_bLoaded`
* Global variables (which should be avoided when possible) should be prefixed with 'g_':
  * `MemManager g_MemManager`
* Pointer and reference variable types should include the pointer or reference marker with the type.
  * Acceptable: `char* ptr`
  * Not acceptable: `char *ptr`

### Enums
* Enum names should be `PascalCase`
* Items in an enum should also be `PascalCase`
* New enums should be defined using `enum class`. Some of the existing enums don't do this and may be corrected in the future.

### Constants
* Constant names should be `ALL_CAPS`
* Prefer `constexpr` over `static const` or `#define`

### Methods
* Method names (both class methods and non-class methods) should be `PascalCase`.
* Parameter names should follow variable naming conventions
* Class methods should be marked `const` if they don't need to modify class member variables.
* Class methods should be marked `override` if they override a virtual method in a base class.
* Class methods may be marked `noexcept` if they don't throw exceptions.
* The preferred order if you want to use more than one is: `const override noexcept`
* Methods not used outside a file should be `static` within the file.
  * The exception to this rule is private class methods, which have to be prototyped in the header file.

### Classes and structs
* `class` and `struct` names should be `PascalCase`
* Use `struct` for data containers. A `struct` should not have any methods beyond setters, getters, and comparison wrappers.
  ```
  struct SubString
  {
      const char* pStart;
      size_t nLength;

      bool IsEmpty() const
      {
          return (nLength == 0);
      }
  }
  ```
* Use `class` for anything requiring additional functionality.
  * Limit code in the header file to templated functions or functions with fewer than five lines. All other code should be placed in the cpp file.
* Limit includes in the header file to those needed by the header file. Headers only needed by the cpp file should be in the cpp file.

### Namespaces
* Namespace names should be `snake_case`.
* The top-level namespace is `ra`.
* `inline` namespaces are not preferred. If you intend the functionality to be at the outer namespace, just put it there.
* Do not use `using` for entire namespaces. It is okay to use `using` for things in a namespace.
  * Acceptable: `using ra::data::Achievement`
  * Not acceptable: `using ra::data`

### Braces
* Braces should appear on a separate line

  Acceptable:
  ```
  if (a == b)
  {
      for (int i = 0; i < 10; i++)
      {
          do_something(i);
      }
  }
  else
  {
      do_something_else();
  }
  ```
  Not acceptable:
  ```
  if (a == b) {
      for (int i = 0; i < 10; i++) {
          do_something(i);
      }
  } else {
      do_something_else();
  }
  ```
* Braces are not required for single statement `if`s
* Single statement `if`s may be placed on the same line if the line is short enough.

  Preferred:
  ```
  if (a == b)
      do_something();
  ```
  Allowed:
  ```
  if (a == b) do_something();
  ```
* Braces should be used consistently across an `if` and its `else` conditions:

  Preferred:
  ```
  if (a == b)
      do_something();
  else
      do_something_else();
  ```
  or
  ```
  if (a == b)
  {
      do_something();
  }
  else
  {
      do_something_else();
  }
  ```
  Acceptable:
  ```
  if (a == b) 
  {
      do_something();
  } 
  else
      do_something_else();
  ```

### Whitespace
* Use spaces instead of tabs
* Indent each block by four spaces
* For `if` and loop statements, a single space should separate the keyword from the parenthesis, and no spaces should exist between the parenthesis and the logic. Spaces should exist between the various parts of the logic.

  Acceptable:
  ```
  if (a == b || c == d)
  ```
  ```
  for (int i = 0; i < 10; ++i)
  ```
  ```
  while (j != 3)
  ```
  Not acceptable:
  ```
  if( a==b || c==d )
  ```
  ```
  for( int i = 0; i < 10; ++i )
  ```
  ```
  while( j != 3 )
  ```
* For function and method calls, spaces should not exist on either side of the parenthesis:

  Acceptable:
  ```
  do_something();
  ```
  ```
  do_something(3, 4);
  ```
  ```
  if (do_something(5))
  ```
  Not acceptable:
  ```
  do_something( );
  ```
  ```
  do_something (3,4)
  ```
  ```
  if( do_something( 5 ) )
  ```

### Switch statements  
* Case labels should be indented one level
* Code should be indented a second level. 
* A single line may be placed between distinct cases.

  Preferred:
  ```
  switch (x)
  {
      case 0:
          do_something();
          break;
          
      case 1:
      case 2:
          do_something_else();
          break;
          
      default:
          do_the_default();
          break;
  }
  ```
  
  Acceptable:
  ```
  switch (x)
  {
      case 0:
          do_something();
          break;
      case 1:
      case 2:
          do_something_else();
          break;
      default:
          do_the_default();
          break;
  }
  ```

  Not acceptable:
  ```
  switch (x)
  {
  case 0:
      do_something();
      break;
  case 1:
  case 2:
      do_something_else();
      break;
  default:
      do_the_default();
      break;
  }
  ```

* Scoping braces should be aligned with the case label and contain the break statement.
  ```
  switch (x)
  {
      case 0:
      {
          int i = do_something();
          do_something_else(i);
          break;
      }
      
      case 1:
      ...
  ```   

* Fallthrough cases must explicitly document the case they fallthrough to.
  ```
  switch (x)
  {
      case 0:
          do_something();
          // fallthrough to case 1
      case 1:
          do_something_else();
          break;
  }
  ```

### File structure
* Headers should be grouped 
  * The primary header for the file (in quotes)
  * Other project headers (in quotes)
  * Non-project headers (in brackets)
  * Within each group, headers should be alphabetized.
* A single newline should exist between the `#include`s and the `namespace`s. 
* Each `namespace` should be on a separate line without newlines between.
* A single newline should exist between the `namespace`s and the first method/variable.
* A single newline should exist between each `method`
* A single newline should exist between the last method and the closing `namespace`s.
* Each closing `namespace` should be on a separate line with a comment indicating the associated namespace without newlines between.
  ```
  #include "MyHeader.h"
  
  #include "LocalHeader1.h"
  #include "OtherHeader.h"
  
  #include <stdlib.h>
  #include <string>
  #include <vector>
  
  namespace ra {
  namespace example {
  
  void Method1(const std::string& sText)
  {
      printf("%s\n", sText.c_str());
  } 
  
  int Method2(const std::vector<int>& vNumbers)
  {
      return vNumbers.empty() ? 0 : vNumbers.at(0);
  }
  
  } // namespace example
  } // namespace ra
  ```