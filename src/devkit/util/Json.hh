#ifndef RA_UTIL_JSON_HH
#define RA_UTIL_JSON_HH
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "services/TextReader.hh"
#include "services/TextWriter.hh"

#undef GetObject

namespace ra {
namespace util {

class Json
{
private:
    class Impl;

public:
    class Reader
    {
    public:
        Reader() noexcept;
        ~Reader();
        Reader(const Reader& other) noexcept = delete;
        Reader& operator=(const Reader&) noexcept = delete;
        Reader(Reader&& other) noexcept = delete;
        Reader& operator=(Reader&&) noexcept = delete;

        /// <summary>
        /// Parses the JSON data.
        /// </summary>
        /// <returns><c>true</c> if parsing was successful, <c>false</c> if not.</returns>
        bool Parse(ra::services::TextReader& pReader);

        /// <summary>
        /// Parses the JSON data.
        /// </summary>
        /// <returns><c>true</c> if parsing was successful, <c>false</c> if not.</returns>
        bool Parse(const std::string& sJson);

        /// <summary>
        /// Gets a human-readable error message describing why <see cref="Parse"/> returned false.
        /// </summary>
        std::string GetParseError() const;
        
        /// <summary>
        /// Gets the offset within the JSON data where the parse error occured.
        /// </summary>
        size_t GetParseErrorOffset() const;

        /// <summary>
        /// Gets the string data from the provided field.
        /// </summary>
        /// <param name="sFieldName">Field to query.</param>
        /// <param name="sDefaultValue">Value to return if the field is not found, or is not a string.</param>
        std::string GetString(const std::string& sFieldName, const std::string& sDefaultValue = "") const;

        /// <summary>
        /// Tries to gets the string data from the provided field.
        /// </summary>
        /// <param name="sFieldName">Field to query.</param>
        /// <param name="sValue">
        /// String to populate with the data from the specified field.
        /// It will not be modified if the field is not found or is not a string.
        /// </param>
        /// <returns><c>true</c> if the value was populated.</returns>
        bool TryGetString(const std::string& sFieldName, std::string& sValue) const;

        /// <summary>
        /// Gets the integer data from the provided field.
        /// </summary>
        /// <param name="sFieldName">Field to query.</param>
        /// <param name="nDefaultValue">Value to return if the field is not found, or is not an integer.</param>
        int GetInteger(const std::string& sFieldName, int nDefaultValue = 0) const;

        /// <summary>
        /// Tries to gets the integer data from the provided field.
        /// </summary>
        /// <param name="sFieldName">Field to query.</param>
        /// <param name="nValue">
        /// Integer to populate with the data from the specified field.
        /// It will not be modified if the field is not found or is not an integer.
        /// </param>
        /// <returns><c>true</c> if the value was populated.</returns>
        bool TryGetInteger(const std::string& sFieldName, int& nValue) const;

        /// <summary>
        /// Gets the integer data from the provided field.
        /// </summary>
        /// <param name="sFieldName">Field to query.</param>
        /// <param name="bDefaultValue">Value to return if the field is not found, or is not an integer.</param>
        bool GetBoolean(const std::string& sFieldName, bool bDefaultValue = false) const;

        /// <summary>
        /// Tries to gets the integer data from the provided field.
        /// </summary>
        /// <param name="sFieldName">Field to query.</param>
        /// <param name="bValue">
        /// Boolean to populate with the data from the specified field.
        /// It will not be modified if the field is not found or is not a boolean.
        /// </param>
        /// <returns><c>true</c> if the value was populated.</returns>
        bool TryGetBoolean(const std::string& sFieldName, bool& bValue) const;

        /// <summary>
        /// Gets the available field names.
        /// </summary>
        /// <returns>Number of fields found.</returns>
        size_t GetKeys(std::vector<std::string>& vKeys) const;

        class Node
        {
        public:
            /// <summary>
            /// Gets the string data from the provided field.
            /// </summary>
            /// <param name="sFieldName">Field to query.</param>
            /// <param name="sDefaultValue">Value to return if the field is not found, or is not a string.</param>
            std::string GetString(const std::string& sFieldName, const std::string& sDefaultValue = "") const;

            /// <summary>
            /// Tries to gets the string data from the provided field.
            /// </summary>
            /// <param name="sFieldName">Field to query.</param>
            /// <param name="sValue">
            /// String to populate with the data from the specified field.
            /// It will not be modified if the field is not found or is not a string.
            /// </param>
            /// <returns><c>true</c> if the value was populated.</returns>
            bool TryGetString(const std::string& sFieldName, std::string& sValue) const;

            /// <summary>
            /// Gets the integer data from the provided field.
            /// </summary>
            /// <param name="sFieldName">Field to query.</param>
            /// <param name="nDefaultValue">Value to return if the field is not found, or is not an integer.</param>
            int GetInteger(const std::string& sFieldName, int nDefaultValue = 0) const;

            /// <summary>
            /// Tries to gets the integer data from the provided field.
            /// </summary>
            /// <param name="sFieldName">Field to query.</param>
            /// <param name="nValue">
            /// Integer to populate with the data from the specified field.
            /// It will not be modified if the field is not found or is not an integer.
            /// </param>
            /// <returns><c>true</c> if the value was populated.</returns>
            bool TryGetInteger(const std::string& sFieldName, int& nValue) const;

            /// <summary>
            /// Gets the integer data from the provided field.
            /// </summary>
            /// <param name="sFieldName">Field to query.</param>
            /// <param name="bDefaultValue">Value to return if the field is not found, or is not an integer.</param>
            bool GetBoolean(const std::string& sFieldName, bool bDefaultValue = false) const;

            /// <summary>
            /// Tries to gets the integer data from the provided field.
            /// </summary>
            /// <param name="sFieldName">Field to query.</param>
            /// <param name="bValue">
            /// Boolean to populate with the data from the specified field.
            /// It will not be modified if the field is not found or is not a boolean.
            /// </param>
            /// <returns><c>true</c> if the value was populated.</returns>
            bool TryGetBoolean(const std::string& sFieldName, bool& bValue) const;

            /// <summary>
            /// Tries to gets the object data from the provided field.
            /// </summary>
            /// <param name="sFieldName">Field to query.</param>
            /// <param name="pObject">
            /// Node to populate with the data from the specified field.
            /// It will not be modified if the field is not found or is not an object.
            /// </param>
            /// <returns><c>true</c> if the value was populated.</returns>
            bool TryGetObject(const std::string& sFieldName, Node& pObject);

            /// <summary>
            /// Gets the available field names.
            /// </summary>
            /// <returns>Number of fields found.</returns>
            size_t GetKeys(std::vector<std::string>& vKeys) const;

        private:
            friend class Reader;
            const void* m_pNode;
        };

        /// <summary>
        /// Tries to gets the object data from the provided field.
        /// </summary>
        /// <param name="sFieldName">Field to query.</param>
        /// <param name="pObject">
        /// Node to populate with the data from the specified field.
        /// It will not be modified if the field is not found or is not an object.
        /// </param>
        /// <returns><c>true</c> if the value was populated.</returns>
        bool TryGetObject(const std::string& sFieldName, Node& pObject);

        /// <summary>
        /// Tries to gets an array of object data from the provided field.
        /// </summary>
        /// <param name="sFieldName">Field to query.</param>
        /// <param name="vObjects">
        /// Array to populate with the data from the specified field.
        /// It will not be modified if the field is not found or is not an array of objects.
        /// </param>
        /// <returns><c>true</c> if the value was populated.</returns>
        bool TryGetObjectArray(const std::string& sFieldName, std::vector<Node>& vObjects);

    private:
        std::unique_ptr<Json::Impl> m_pDocument;
    };

    class Writer
    {
    public:
        Writer() noexcept;
        ~Writer();
        Writer(const Writer& other) noexcept = delete;
        Writer& operator=(const Writer&) noexcept = delete;
        Writer(Writer&& other) noexcept = delete;
        Writer& operator=(Writer&&) noexcept = delete;

        /// <summary>
        /// Writes the constructed JSON data.
        /// </summary>
        /// <returns><c>true</c> if writing was successful, <c>false</c> if not.</returns>
        bool Save(ra::services::TextWriter& pWriter);

        /// <summary>
        /// Sets a field to a string value.
        /// </summary>
        void SetString(const std::string& sFieldName, const std::string& sValue);

        /// <summary>
        /// Sets a field to an integer value.
        /// </summary>
        void SetInteger(const std::string& sFieldName, int nValue);

        /// <summary>
        /// Sets a field to a boolean value.
        /// </summary>
        void SetBoolean(const std::string& sFieldName, bool bValue);

        class Node
        {
        public:
            /// <summary>
            /// Sets a field to a string value.
            /// </summary>
            void SetString(const std::string& sFieldName, const std::string& sValue);

            /// <summary>
            /// Sets a field to an integer value.
            /// </summary>
            void SetInteger(const std::string& sFieldName, int nValue);

            /// <summary>
            /// Sets a field to a boolean value.
            /// </summary>
            void SetBoolean(const std::string& sFieldName, bool bValue);

            /// <summary>
            /// Returns an object to set fields in a nested object.
            /// </summary>
            Node SetObject(const std::string& sFieldName);

            /// <summary>
            /// Returns an object to hold an array of objects.
            /// </summary>
            Node SetObjectArray(const std::string& sFieldName);

            /// <summary>
            /// Returns an object appended to an array of objects.
            /// </summary>
            Node AppendObject();

        private:
            friend class Writer;
            void* m_pNode;
            Writer* m_pOwner; 
        };

        /// <summary>
        /// Sets a field to a string value.
        /// </summary>
        Node SetObject(const std::string& sFieldName);

        /// <summary>
        /// Returns an object to hold an array of objects.
        /// </summary>
        Node SetObjectArray(const std::string& sFieldName);

    private:
        std::unique_ptr<Json::Impl> m_pDocument;
    };
};

} // namespace util
} // namespace ra

#endif // !RA_UTIL_JSON_HH
