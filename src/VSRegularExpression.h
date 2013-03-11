/* vim: set ft=objc fenc=utf-8 sw=2 ts=2 et: */
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

#import <Foundation/Foundation.h>

typedef NS_OPTIONS(NSUInteger, VSRegularExpressionOptions) {
  VSRegularExpressionCaseInsensitive = 1 << 0,
  VSRegularExpressionAnchorsMatchLines = 1 << 1,
  VSRegularExpressionMatchGlobally = 1 << 2
};

@interface VSRegularExpression : NSObject <NSCopying, NSCoding>

+ (VSRegularExpression *)regularExpressionWithPattern:(NSString *)pattern
                                              options:(VSRegularExpressionOptions)options
                                                error:(NSError **)error;

- (id)initWithPattern:(NSString *)pattern
              options:(VSRegularExpressionOptions)options
                error:(NSError **)error;

@property (nonatomic, readonly) NSString *pattern;
@property (nonatomic, readonly) VSRegularExpressionOptions options;
@property (nonatomic, readonly) NSUInteger numberOfCaptureGroups;

@end

@interface VSRegularExpression (Matching)

- (void)enumerateMatchesInString:(NSString *)string usingBlock:(void (^)(NSArray *result, BOOL *stop))block;
- (void)enumerateMatchesInString:(NSString *)string range:(NSRange)range usingBlock:(void (^)(NSArray *result, BOOL *stop))block;

- (NSArray *)matchesInString:(NSString *)string;
- (NSUInteger)numberOfMatchesInString:(NSString *)string;

- (NSArray *)matchesInString:(NSString *)string range:(NSRange)range;
- (NSUInteger)numberOfMatchesInString:(NSString *)string range:(NSRange)range;

@end

@interface VSRegularExpression (Replacement)

- (NSString *)stringByReplacingMatchesInString:(NSString *)string withTemplate:(NSString *)templ;
- (NSString *)stringByReplacingMatchesInString:(NSString *)string range:(NSRange)range withTemplate:(NSString *)templ;

@end
