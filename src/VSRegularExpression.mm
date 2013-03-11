/* vim: set ft=objcpp fenc=utf-8 sw=2 ts=2 et: */
/*
 * Copyright (c) 2013 Chongyu Zhu <lembacon@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#import "VSRegularExpression.h"
#include "jscre/regexp.h"

namespace {

void nsstring_get_utf16(NSString *string, uint16_t **utf16, size_t *utf16_length)
{
  if (string == nil) {
    *utf16 = nullptr;
    *utf16_length = 0;
    return;
  }

  const UniChar *characters = CFStringGetCharactersPtr((__bridge CFStringRef)string);
  const CFIndex length = CFStringGetLength((__bridge CFStringRef)string);
  UniChar *buffer = static_cast<UniChar *>(malloc(sizeof(UniChar) * length));

  if (characters == nullptr) {
    CFStringGetCharacters((__bridge CFStringRef)string, CFRangeMake(0, length), buffer);
  }
  else {
    memcpy(buffer, characters, sizeof(UniChar) * length);
  }

  *utf16 = static_cast<uint16_t *>(buffer);
  *utf16_length = static_cast<size_t>(length);
}

NSString *const kPatternCodingKey = @"Pattern";
NSString *const kOptionsCodingKey = @"Options";

NSString *const kErrorDomain = @"VSRegularExpressionErrorDomain";

} // end namespace

@interface VSRegularExpression () {
@private
  NSString *_pattern;
  VSRegularExpressionOptions _options;
  NSUInteger _numberOfCaptureGroups;
  jscre::regexp::RegExpPtr _regExp;
}
@end

@implementation VSRegularExpression

@synthesize pattern = _pattern;
@synthesize options = _options;
@synthesize numberOfCaptureGroups = _numberOfCaptureGroups;

- (id)copyWithZone:(NSZone *)zone
{
  return [[[self class] allocWithZone:zone] initWithPattern:_pattern
                                                    options:_options
                                                      error:nullptr];
}

- (void)encodeWithCoder:(NSCoder *)aCoder
{
  [aCoder encodeObject:_pattern forKey:kPatternCodingKey];
  [aCoder encodeInteger:static_cast<NSInteger>(_options) forKey:kOptionsCodingKey];
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
  return [[[self class] alloc] initWithPattern:[aDecoder decodeObjectForKey:kPatternCodingKey]
                                       options:static_cast<VSRegularExpressionOptions>([aDecoder decodeIntegerForKey:kOptionsCodingKey])
                                         error:nullptr];
}

+ (VSRegularExpression *)regularExpressionWithPattern:(NSString *)pattern
                                              options:(VSRegularExpressionOptions)options
                                                error:(NSError **)error
{
  return [[[self class] alloc] initWithPattern:pattern
                                       options:options
                                         error:error];
}

- (id)initWithPattern:(NSString *)pattern
              options:(VSRegularExpressionOptions)options
                error:(NSError **)error
{
  self = [super init];
  if (!self) {
    if (error != nullptr) {
      *error = [NSError errorWithDomain:kErrorDomain
                                   code:0
                               userInfo:@{NSLocalizedDescriptionKey: @"Failed to initialize."}];
    }
    return nil;
  }

  uint16_t *patternRaw;
  size_t patternRawLength;
  nsstring_get_utf16(pattern, &patternRaw, &patternRawLength);
  if (patternRaw == nullptr) {
    if (error != nullptr) {
      *error = [NSError errorWithDomain:kErrorDomain
                                   code:0
                               userInfo:@{NSLocalizedDescriptionKey: @"Pattern is nil."}];
    }
    return nil;
  }

  bool global = options & VSRegularExpressionMatchGlobally;
  bool multiline = options & VSRegularExpressionAnchorsMatchLines;
  bool ignoreCase = options & VSRegularExpressionCaseInsensitive;
  jscre::regexp::RegExpPtr re = std::make_shared<jscre::regexp::RegExp>(patternRaw,
                                                                        patternRawLength,
                                                                        global,
                                                                        multiline,
                                                                        ignoreCase);

  free(patternRaw);

  if (re == nullptr) {
    if (error != nullptr) {
      *error = [NSError errorWithDomain:kErrorDomain
                                   code:0
                               userInfo:@{NSLocalizedDescriptionKey: @"Failed to compile pattern."}];
    }
    return nil;
  }
  else if (re->hasError()) {
    if (error != nullptr) {
      *error = [NSError errorWithDomain:kErrorDomain
                                   code:0
                               userInfo:@{NSLocalizedDescriptionKey: @(re->getErrorMessage().c_str())}];
    }
    return nil;
  }

  _pattern = [pattern copy];
  _options = options;
  _numberOfCaptureGroups = static_cast<NSUInteger>(re->getStorageCount());
  _regExp = std::move(re);

  if (error != nullptr) {
    *error = nil;
  }

  return self;
}

- (NSString *)_descriptionForOptions
{
  NSMutableArray *array = [NSMutableArray array];
  if (_options & VSRegularExpressionCaseInsensitive) {
    [array addObject:@"CaseInsensitive"];
  }
  if (_options & VSRegularExpressionAnchorsMatchLines) {
    [array addObject:@"AnchorsMatchLines"];
  }
  if (_options & VSRegularExpressionMatchGlobally) {
    [array addObject:@"MatchGlobally"];
  }

  if ([array count] == 0) {
    return @"None";
  }

  return [array componentsJoinedByString:@", "];
}

- (NSString *)description
{
  NSMutableString *desc = [NSMutableString string];
  [desc appendFormat:@"%@\n", [super description]];
  [desc appendFormat:@"Pattern: %@\n", _pattern];
  [desc appendFormat:@"Options: %@\n", [self _descriptionForOptions]];
  [desc appendFormat:@"\n%s", _regExp->toString().c_str()];
  return [desc copy];
}

@end

@implementation VSRegularExpression (Matching)

- (void)enumerateMatchesInString:(NSString *)string usingBlock:(void (^)(NSArray *result, BOOL *stop))block
{
  [self enumerateMatchesInString:string range:NSMakeRange(0, [string length]) usingBlock:block];
}

- (void)enumerateMatchesInString:(NSString *)string range:(NSRange)range usingBlock:(void (^)(NSArray *result, BOOL *stop))block
{
  [[self matchesInString:string range:range] enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
    block(obj, stop);
  }];
}

- (NSArray *)matchesInString:(NSString *)string
{
  return [self matchesInString:string range:NSMakeRange(0, [string length])];
}

- (NSUInteger)numberOfMatchesInString:(NSString *)string
{
  return [self numberOfMatchesInString:string range:NSMakeRange(0, [string length])];
}

- (NSArray *)matchesInString:(NSString *)string range:(NSRange)range
{
  NSString *input = [string substringWithRange:range];

  uint16_t *inputRaw;
  size_t inputRawLength;
  nsstring_get_utf16(input, &inputRaw, &inputRawLength);

  if (inputRaw == nullptr) {
    return [NSArray array];
  }

  jscre::regexp::MatchVector &&allMatches = _regExp->execAll(inputRaw, inputRawLength);
  free(inputRaw);

  NSMutableArray *matches = [NSMutableArray array];
  for (auto &m: allMatches) {
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:static_cast<NSUInteger>(m->getCapturedCount())];
    for (size_t i = 0; i < m->getCapturedCount(); ++i) {
      NSUInteger start = static_cast<NSUInteger>(m->getCapturedTextIndex(i));
      NSUInteger length = static_cast<NSUInteger>(m->getCapturedTextLength(i));
      NSString *capturedText = [input substringWithRange:NSMakeRange(start, length)];
      [result addObject:capturedText];
    }

    [matches addObject:result];
  }

  return matches;
}

- (NSUInteger)numberOfMatchesInString:(NSString *)string range:(NSRange)range
{
  return [[self matchesInString:string range:range] count];
}

@end

@implementation VSRegularExpression (Replacement)

- (NSString *)stringByReplacingMatchesInString:(NSString *)string withTemplate:(NSString *)templ
{
  return [self stringByReplacingMatchesInString:string range:NSMakeRange(0, [string length]) withTemplate:templ];
}

- (NSString *)stringByReplacingMatchesInString:(NSString *)string range:(NSRange)range withTemplate:(NSString *)templ
{
  NSString *input = [string substringWithRange:range];

  uint16_t *inputRaw;
  size_t inputRawLength;
  nsstring_get_utf16(input, &inputRaw, &inputRawLength);

  uint16_t *templRaw;
  size_t templRawLength;
  nsstring_get_utf16(templ, &templRaw, &templRawLength);

  if (inputRaw == nullptr || templRaw == nullptr) {
    if (inputRaw != nullptr) {
      free(inputRaw);
    }
    if (templRaw != nullptr) {
      free(templRaw);
    }
    return input;
  }

  uint16_t *outputRaw = nullptr;
  size_t outputRawLength = 0;

  jscre::regexp::replace(*_regExp, templRaw, templRawLength, inputRaw, inputRawLength, outputRaw, outputRawLength);
  free(inputRaw);
  free(templRaw);

  if (outputRaw == nullptr) {
    return input;
  }

  return CFBridgingRelease(CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
                                                              static_cast<const UniChar *>(outputRaw),
                                                              static_cast<CFIndex>(outputRawLength),
                                                              kCFAllocatorMalloc));
}

@end
