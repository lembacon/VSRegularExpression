# VSRegularExpression

_VSRegularExpression_ is a JavaScript-compatible (with some limitations and known issues) Regular Expression implementation written in C++ with Objective-C encapsulation similar to _NSRegularExpression_.

This whole thing is written purely for fun, though you can still use it when the input is not too large and you know what you are doing.

(The RegExp engine written in C++ is named _jscre_. Though I soon realized that the _V8_'s port of _pcre_ was also named _jscre_, I'm too lazy to give it another name.)

## Limitations and Known Issues

- Extremely poor performance.
- Always return the longest possible match.
- UTF-16 surrogate pairs are not well-handled.
- _Backreference_ is not supported.
- _Non-greedy quantification_ is not supported.
- Only ASCII characters are considered when using either _case insensitive_ or _word boundary_.

## License

_VSRegularExpression_ is licensed under [MIT License](https://github.com/lembacon/VSRegularExpression/blob/master/LICENSE).