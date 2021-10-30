#pragma once

namespace mysql {

// Header information
enum : uint8_t {
    OKHeader = 0x00,
    ErrHeader = 0xff,
    EOFHeader = 0xfe,
    LocalInFileHeader = 0xfb,
};

// Protocol Features
enum {
    AuthSwitchRequest = 0xfe,
};

// Server information.
enum : uint16_t {
    ServerStatusInTrans = 0x0001,
    ServerStatusAutocommit = 0x0002,
    ServerMoreResultsExists = 0x0008,
    ServerStatusNoGoodIndexUsed = 0x0010,
    ServerStatusNoIndexUsed = 0x0020,
    ServerStatusCursorExists = 0x0040,
    ServerStatusLastRowSend = 0x0080,
    ServerStatusDBDropped = 0x0100,
    ServerStatusNoBackslashEscaped = 0x0200,
    ServerStatusMetadataChanged = 0x0400,
    ServerStatusWasSlow = 0x0800,
    ServerPSOutParams = 0x1000,
};

// Identifier length limitations.
// See https://dev.mysql.com/doc/refman/5.7/en/identifiers.html
enum {
    // MaxPayloadLen is the max packet payload length.
    MaxPayloadLen = (1 << 24) - 1,
    // MaxTableNameLength is max length of table name identifier.
    MaxTableNameLength = 64,
    // MaxDatabaseNameLength is max length of database name identifier.
    MaxDatabaseNameLength = 64,
    // MaxColumnNameLength is max length of column name identifier.
    MaxColumnNameLength = 64,
    // MaxKeyParts is max length of key parts.
    MaxKeyParts = 16,
    // MaxIndexIdentifierLen is max length of index identifier.
    MaxIndexIdentifierLen = 64,
    // MaxConstraintIdentifierLen is max length of constrain identifier.
    MaxConstraintIdentifierLen = 64,
    // MaxViewIdentifierLen is max length of view identifier.
    MaxViewIdentifierLen = 64,
    // MaxAliasIdentifierLen is max length of alias identifier.
    MaxAliasIdentifierLen = 256,
    // MaxUserDefinedVariableLen is max length of user-defined variable.
    MaxUserDefinedVariableLen = 64,
};

// ErrTextLength error text length limit.
enum {
    ErrTextLength = 80,
};

// consts for sql modes.
// see https://dev.mysql.com/doc/internals/en/query-event.html#q-sql-mode-code
enum : uint64_t {
    ModeRealAsFloat = 1ul << 0,
    ModePipesAsConcat = 1ul << 2,
    ModeANSIQuotes = 1ul << 3,
    ModeIgnoreSpace = 1ul << 4,
    ModeNotUsed = 1ul << 5,
    ModeOnlyFullGroupBy = 1ul << 6,
    ModeNoUnsignedSubtraction = 1ul << 7,
    ModeNoDirInCreate = 1ul << 8,
    ModePostgreSQL = 1ul << 9,
    ModeOracle = 1ul << 10,
    ModeMsSQL = 1ul << 11,
    ModeDb2 = 1ul << 12,
    ModeMaxdb = 1ul << 13,
    ModeNoKeyOptions = 1ul << 14,
    ModeNoTableOptions = 1ul << 15,
    ModeNoFieldOptions = 1ul << 16,
    ModeMySQL323 = 1ul << 17,
    ModeMySQL40 = 1ul << 18,
    ModeANSI = 1ul << 19,
    ModeNoAutoValueOnZero = 1ul << 20,
    ModeNoBackslashEscapes = 1ul << 21,
    ModeStrictTransTables = 1ul << 22,
    ModeStrictAllTables = 1ul << 23,
    ModeNoZeroInDate = 1ul << 24,
    ModeNoZeroDate = 1ul << 25,
    ModeInvalidDates = 1ul << 26,
    ModeErrorForDivisionByZero = 1ul << 27,
    ModeTraditional = 1ul << 28,
    ModeNoAutoCreateUser = 1ul << 29,
    ModeHighNotPrecedence = 1ul << 30,
    ModeNoEngineSubstitution = 1ul << 31,
    ModePadCharToFullLength = 1ul << 32,
    ModeAllowInvalidDates = 1ul << 33,
    ModeNone = 0,
};

// SQLMode is the type for MySQL sql_mode.
// See https://dev.mysql.com/doc/refman/5.7/en/sql-mode.html
struct SQLMode {
    uint64_t _mode;
    // HasNoZeroDateMode detects if 'NO_ZERO_DATE' mode is set in SQLMode
    bool HasNoZeroDateMode() { return (_mode & ModeNoZeroDate) == ModeNoZeroDate; }

    // HasNoZeroInDateMode detects if 'NO_ZERO_IN_DATE' mode is set in SQLMode
    bool HasNoZeroInDateMode() { return (_mode & ModeNoZeroInDate) == ModeNoZeroInDate; }

    // HasErrorForDivisionByZeroMode detects if 'ERROR_FOR_DIVISION_BY_ZERO' mode is set in SQLMode
    bool HasErrorForDivisionByZeroMode() { return (_mode & ModeErrorForDivisionByZero) == ModeErrorForDivisionByZero; }

    // HasOnlyFullGroupBy detects if 'ONLY_FULL_GROUP_BY' mode is set in SQLMode
    bool HasOnlyFullGroupBy() { return (_mode & ModeOnlyFullGroupBy) == ModeOnlyFullGroupBy; }

    // HasStrictMode detects if 'STRICT_TRANS_TABLES' or 'STRICT_ALL_TABLES' mode is set in SQLMode
    bool HasStrictMode() {
        return (_mode & ModeStrictTransTables) == ModeStrictTransTables ||
               (_mode & ModeStrictAllTables) == ModeStrictAllTables;
    }

    // HasPipesAsConcatMode detects if 'PIPES_AS_CONCAT' mode is set in SQLMode
    bool HasPipesAsConcatMode() { return (_mode & ModePipesAsConcat) == ModePipesAsConcat; }

    // HasNoUnsignedSubtractionMode detects if 'NO_UNSIGNED_SUBTRACTION' mode is set in SQLMode
    bool HasNoUnsignedSubtractionMode() { return (_mode & ModeNoUnsignedSubtraction) == ModeNoUnsignedSubtraction; }

    // HasHighNotPrecedenceMode detects if 'HIGH_NOT_PRECEDENCE' mode is set in SQLMode
    bool HasHighNotPrecedenceMode() { return (_mode & ModeHighNotPrecedence) == ModeHighNotPrecedence; }

    // HasANSIQuotesMode detects if 'ANSI_QUOTES' mode is set in SQLMode
    bool HasANSIQuotesMode() { return (_mode & ModeANSIQuotes) == ModeANSIQuotes; }

    // HasRealAsFloatMode detects if 'REAL_AS_FLOAT' mode is set in SQLMode
    bool HasRealAsFloatMode() { return (_mode & ModeRealAsFloat) == ModeRealAsFloat; }

    // HasPadCharToFullLengthMode detects if 'PAD_CHAR_TO_FULL_LENGTH' mode is set in SQLMode
    bool HasPadCharToFullLengthMode() { return (_mode & ModePadCharToFullLength) == ModePadCharToFullLength; }

    // HasNoBackslashEscapesMode detects if 'NO_BACKSLASH_ESCAPES' mode is set in SQLMode
    bool HasNoBackslashEscapesMode() { return (_mode & ModeNoBackslashEscapes) == ModeNoBackslashEscapes; }

    // HasIgnoreSpaceMode detects if 'IGNORE_SPACE' mode is set in SQLMode
    bool HasIgnoreSpaceMode() { return (_mode & ModeIgnoreSpace) == ModeIgnoreSpace; }

    // HasNoAutoCreateUserMode detects if 'NO_AUTO_CREATE_USER' mode is set in SQLMode
    bool HasNoAutoCreateUserMode() { return (_mode & ModeNoAutoCreateUser) == ModeNoAutoCreateUser; }

    // HasAllowInvalidDatesMode detects if 'ALLOW_INVALID_DATES' mode is set in SQLMode
    bool HasAllowInvalidDatesMode() { return (_mode & ModeAllowInvalidDates) == ModeAllowInvalidDates; }
};
}  // namespace mysql