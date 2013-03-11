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

#import "VSRegularExpressionTests.h"
#import "VSRegularExpression.h"

@implementation VSRegularExpressionTests

- (void)setUp
{
  [super setUp];

  // Set-up code here.
}

- (void)tearDown
{
  // Tear-down code here.

  [super tearDown];
}

- (void)testCompilationError
{
  STAssertNil([VSRegularExpression regularExpressionWithPattern:@"[[[[[[wtf]]" options:0 error:NULL], nil);
  STAssertNil([VSRegularExpression regularExpressionWithPattern:@"(" options:0 error:NULL], nil);
  STAssertNotNil([VSRegularExpression regularExpressionWithPattern:@"()" options:0 error:NULL], nil);
  STAssertNotNil([VSRegularExpression regularExpressionWithPattern:@"" options:0 error:NULL], nil);
}

- (void)testLookAhead
{
  VSRegularExpression *re = [VSRegularExpression regularExpressionWithPattern:@"Hello(?=World)"
                                                                      options:0
                                                                        error:NULL];

  STAssertTrue([re numberOfMatchesInString:@"Hello"] == 0, nil);
  STAssertTrue([re numberOfMatchesInString:@"HelloWTF"] == 0, nil);
  STAssertTrue([re numberOfMatchesInString:@"HelloWorld"] > 0, nil);
  STAssertEqualObjects([re matchesInString:@"HelloWorld"][0][0], @"Hello", nil);
}

- (void)testInverseLookAhead
{
  VSRegularExpression *re = [VSRegularExpression regularExpressionWithPattern:@"Hello(?!World)"
                                                                      options:0
                                                                        error:NULL];

  STAssertTrue([re numberOfMatchesInString:@"Hello"] > 0, nil);
  STAssertTrue([re numberOfMatchesInString:@"HelloWTF"] > 0, nil);
  STAssertTrue([re numberOfMatchesInString:@"HelloWorld"] == 0, nil);
  STAssertEqualObjects([re matchesInString:@"Hello"][0][0], @"Hello", nil);
  STAssertEqualObjects([re matchesInString:@"HelloWTF"][0][0], @"Hello", nil);
}

- (void)testNumberOfCaptureGroups
{
  STAssertEquals([[VSRegularExpression regularExpressionWithPattern:@""
                                                            options:0
                                                              error:NULL] numberOfCaptureGroups], 0UL, nil);

  STAssertEquals([[VSRegularExpression regularExpressionWithPattern:@"(wtf(a(?:b)))"
                                                            options:0
                                                              error:NULL] numberOfCaptureGroups], 2UL, nil);

  STAssertEquals([[VSRegularExpression regularExpressionWithPattern:@"(?:wtf(a(?:b)))"
                                                            options:0
                                                              error:NULL] numberOfCaptureGroups], 1UL, nil);
}

- (void)testCapture
{
  VSRegularExpression *re = [VSRegularExpression regularExpressionWithPattern:@"VSRegular((?:(Expression)( ))(Rocks))"
                                                                      options:0
                                                                        error:NULL];

  NSString *input = @"VSRegularExpression Rocks";
  STAssertEquals([re numberOfMatchesInString:input], 1UL, nil);
  STAssertEquals([[re matchesInString:input][0] count], 5UL, nil);
  STAssertEquals([re numberOfCaptureGroups], 4UL, nil);
  STAssertEqualObjects([re matchesInString:input][0][0], input, nil);
  STAssertEqualObjects([re matchesInString:input][0][1], @"Expression Rocks", nil);
  STAssertEqualObjects([re matchesInString:input][0][2], @"Expression", nil);
  STAssertEqualObjects([re matchesInString:input][0][3], @" ", nil);
  STAssertEqualObjects([re matchesInString:input][0][4], @"Rocks", nil);
}

- (void)testReplacement
{
  VSRegularExpression *re = [VSRegularExpression regularExpressionWithPattern:@"\"([a-zA-Z_][a-zA-Z0-9_]*)\""
                                                                      options:VSRegularExpressionMatchGlobally
                                                                        error:NULL];

  STAssertEqualObjects([re stringByReplacingMatchesInString:@"\"left\"=\"right\""
                                               withTemplate:@"$1"], @"left=right", nil);
  STAssertEqualObjects([re stringByReplacingMatchesInString:@"\"left\"=\"right\""
                                               withTemplate:@"$$$1$$"], @"$left$=$right$", nil);
}

@end
