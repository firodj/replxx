#ifdef _WIN32

#include <conio.h>
#include <windows.h>
#include <io.h>
#if _MSC_VER < 1900 && defined (_MSC_VER)
#define snprintf _snprintf	// Microsoft headers use underscores in some names
#endif
#define strcasecmp _stricmp
#define strdup _strdup
#define write _write
#define STDIN_FILENO 0

#else /* _WIN32 */

#include <unistd.h>

#endif /* _WIN32 */

#include "prompt.hxx"
#include "util.hxx"
#include "io.hxx"

namespace replxx {

Prompt::Prompt( void )
	: _extraLines( 0 )
	, _lastLinePosition( 0 )
	, _previousInputLen( 0 )
	, _previousLen( 0 )
	, _screenColumns( 0 ) {
	update_screen_columns();
}

void Prompt::write() {
	write32( _text.get(), _byteCount );
}

void Prompt::update_screen_columns( void ) {
	_screenColumns = getScreenColumns();
}

void Prompt::set_text( std::string const& text_ ) {
	UnicodeString tempUnicode( text_ );

	// strip control characters from the prompt -- we do allow newline
	UnicodeString::const_iterator in( tempUnicode.begin() );
	UnicodeString::iterator out( tempUnicode.begin() );

	int len = 0;
	int x = 0;

	bool const strip = !tty::out;

	while (in != tempUnicode.end()) {
		char32_t c = *in;
		if ('\n' == c || !isControlChar(c)) {
			*out = c;
			++out;
			++in;
			++len;
			if ('\n' == c || ++x >= _screenColumns) {
				x = 0;
				++_extraLines;
				_lastLinePosition = len;
			}
		} else if (c == '\x1b') {
			if ( strip ) {
				// jump over control chars
				++in;
				if (*in == '[') {
					++in;
					while ( ( in != tempUnicode.end() ) && ( ( *in == ';' ) || ( ( ( *in >= '0' ) && ( *in <= '9' ) ) ) ) ) {
						++in;
					}
					if (*in == 'm') {
						++in;
					}
				}
			} else {
				// copy control chars
				*out = *in;
				++out;
				++in;
				if (*in == '[') {
					*out = *in;
					++out;
					++in;
					while ( ( in != tempUnicode.end() ) && ( ( *in == ';' ) || ( ( ( *in >= '0' ) && ( *in <= '9' ) ) ) ) ) {
						*out = *in;
						++out;
						++in;
					}
					if (*in == 'm') {
						*out = *in;
						++out;
						++in;
					}
				}
			}
		} else {
			++in;
		}
	}
	_characterCount = len;
	_byteCount = static_cast<int>(out - tempUnicode.begin());
	_text = tempUnicode;

	_indentation = len - _lastLinePosition;
	_cursorRowOffset = _extraLines;
}

// Used with DynamicPrompt (history search)
//
const UnicodeString forwardSearchBasePrompt("(i-search)`");
const UnicodeString reverseSearchBasePrompt("(reverse-i-search)`");
const UnicodeString endSearchBasePrompt("': ");
UnicodeString previousSearchText;	// remembered across invocations of replxx_input()

DynamicPrompt::DynamicPrompt( int initialDirection )
	: Prompt()
	, _searchText()
	, _direction( initialDirection ) {
	_cursorRowOffset = 0;
	const UnicodeString* basePrompt =
			(_direction > 0) ? &forwardSearchBasePrompt : &reverseSearchBasePrompt;
	size_t promptStartLength = basePrompt->length();
	_characterCount = static_cast<int>(promptStartLength + endSearchBasePrompt.length());
	_byteCount = _characterCount;
	_lastLinePosition = _characterCount; // TODO fix this, we are asssuming
	                                        // that the history prompt won't wrap (!)
	_previousLen = _characterCount;
	_text.assign( *basePrompt ).append( endSearchBasePrompt );
	calculateScreenPosition(
		0, 0, screen_columns(), _characterCount,
		_indentation, _extraLines
	);
}

void DynamicPrompt::updateSearchPrompt(void) {
	const UnicodeString* basePrompt =
			(_direction > 0) ? &forwardSearchBasePrompt : &reverseSearchBasePrompt;
	size_t promptStartLength = basePrompt->length();
	_characterCount = static_cast<int>(promptStartLength + _searchText.length() +
																 endSearchBasePrompt.length());
	_byteCount = _characterCount;
	_text.assign( *basePrompt ).append( _searchText ).append( endSearchBasePrompt );
}

}

