#include <iostream>
using namespace std;

enum Rule { /* CONTAINS ALL RULES THAT APPEAR IN PARSE TABLE */
    // Success, Fail signals for the parser
    ACCEPT = -9999, ERROR = -10000,
    ERROR00 = -10001, ERROR01 = -10002, ERROR02 = -10003, ERROR03 = -10004, ERROR04 = -10005, ERROR05 = -10006,
    ERROR06 = -10007, ERROR07 = -10008, ERROR08 = -10009, ERROR09 = -10010, ERROR10 = -10011, ERRORMAX = -10100,

    // Reduce-to derivation rule numbers ( denoted by negative of the rule number )
    RULE01 = -1, RULE02 = -2, RULE03 = -3, RULE04 = -4, RULE05 = -5, RULE06 = -6, RULEMAX = -999,

    // Shift-to row number ( denoted by positive integer corresponding to the row)
    STATE00 = 0, STATE01 = 1, STATE02 = 2, STATE03 = 3, STATE04 = 4, STATE05 = 5, STATE06 = 6,
    STATE07 = 7, STATE08 = 8, STATE09 = 9, STATE10 = 10, STATE11 = 11, STATEMAX = 999,

    // Terminals in the grammar
    tDOLLAR = 2200, tPLUS = 2201, tSTAR = 2202, tOPENPAR = 2203, tCLOSEPAR = 2204, tNUM = 2205,

    // Variables in the grammar
    vE = 2206, vT = 2207, vM = 2208
};

const Rule parseTable[12][9] = {
    /*
    GRAMMAR:
        (1) E -> E + T      (2) E -> T
        (3) T → T * M       (4) T -> M
        (5) M → ( E )       (6) M -> num
    */
    //                                ACTION                                              GOTO
    //            $           +          *           (         )          num          E         T          M
    /* 00 */ { ERROR00,   ERROR01,    ERROR01,    STATE04,  ERROR01,    STATE05,    STATE01,  STATE02,   STATE03  },
    /* 01 */ { ACCEPT,    STATE06,    ERROR02,    ERROR02,  ERROR02,    ERROR,      ERROR,    ERROR,     ERROR    },
    /* 02 */ { RULE02,    RULE02,     STATE07,    ERROR03,  RULE02,     ERROR03,    ERROR,    ERROR,     ERROR    },  // reduce to E
    /* 03 */ { RULE04,    RULE04,     RULE04,     ERROR04,  RULE04,     ERROR04,    ERROR,    ERROR,     ERROR    },

    /* 04 */ { ERROR00,   ERROR01,    ERROR01,    STATE04,  ERROR01,    STATE05,    STATE08,  STATE02,   STATE03  },
    /* 05 */ { RULE06,    RULE06,     RULE06,     ERROR06,  RULE06,     ERROR06,    ERROR,    ERROR,     ERROR    },
    /* 06 */ { ERROR00,   ERROR01,    ERROR01,    STATE04,  ERROR01,    STATE05,    ERROR,    STATE09,   STATE03  },
    /* 07 */ { ERROR00,   ERROR01,    ERROR01,    STATE04,  ERROR01,    STATE05,    ERROR,    ERROR,     STATE10  },

    /* 08 */ { ERROR00,   STATE06,    ERROR07,    ERROR07,  STATE11,    ERROR07,    ERROR,    ERROR,     ERROR    },
    /* 09 */ { RULE01,    RULE01,     STATE07,    ERROR08,  RULE01,     ERROR08,    ERROR,    ERROR,     ERROR    },
    /* 10 */ { RULE03,    RULE03,     RULE03,     ERROR09,  RULE03,     ERROR09,    ERROR,    ERROR,     ERROR    },
    /* 11 */ { RULE05,    RULE05,     RULE05,     ERROR10,  RULE05,     ERROR10,    ERROR,    ERROR,     ERROR    },
};

int const TOKEN_VAL_NaN = -999999;
struct Token {
    int tokenVal;
    Rule identity;
};
struct TokenArray {
    Token tokenArr[30];
    int size = 0;

    void emplace_back( int value ) {
        tokenArr[ size ].tokenVal = value;
        tokenArr[ size ].identity = tNUM;
        size++;
    }
    void emplace_back( Rule symbol ) {
        tokenArr[ size ].tokenVal = TOKEN_VAL_NaN;
        tokenArr[ size ].identity = symbol;
        size++;
    }
};
struct Stack {
    Rule stackArray[30];
    int topIndex;

    Stack() {
        topIndex = 0;
        stackArray[ topIndex ] = STATE00;
    }
    void push( Rule state ) {
        stackArray[ ++topIndex ] = state;
        // cout << "\npushed ";print();
    }
    void pop( int popNum = 1 ) {
        topIndex -= popNum;
        // cout << "\npopped " << popNum << ": "; print();
    }
    Rule top() {
        // cout << "\ntop is " << stackArray[ topIndex ] << endl;
        return stackArray[ topIndex ];
    }
    void print() {
        for ( int i = 0; i <= topIndex; i++ )
            cout << stackArray[i] << " ";
        cout << endl;
    }
};

TokenArray tokenise( string inputString, bool& error );
void shift( Rule act, Token lookahead, Stack& stateStack, int& i, int& k, int sdtResultBuffer[10] );
void reduce( Stack& stateStack, int sdtResultBuffer[2], int& k, const Rule derivationRule );
bool parse( string input );
int parseDigitVal( int digits[8], int digitCount );
int power10( unsigned int y );
void printInputError( string inputString, int position );
string getLookaheadChar( Rule lookahead );
void errorHandler( const Rule errorCode, const Token lookahead, Stack stateStack, int& i, int& k, int sdtResultBuffer[], int numOfTokens );

int main()
{
/*
    string inputString;
    cout << "Enter string to parse (max 30 chars from { (,), +, *, n }):\n";
     cin >> inputString;

    cout << parse( inputString ) ? "success" : "fail";
*/

    //parse( "1+2" );                         cout << "\t Expected: SUCCESS\n";
    //parse( "(1+2)*3" );                     cout << "\t Expected: SUCCESS\n";
    //parse( "((1+2)*3(" );                   cout << "\t Expected: FAIL\n";
    //parse( "(1+2)*(3+13)+13" );             cout << "\t Expected: SUCCESS\n";
    //parse( "(1+2)*(13+3)(2*(1+3)" );        cout << "\t Expected: FAIL\n";
    //parse( "2*7+1(4" );
    parse( "10*100*10*(4+3)");
    return 0;
}


bool parse( string inputString ) {
    cout << "Parsing string:  " << inputString << endl;

    // Tokenise input, abort if failed
    bool errorFlag = false;
    TokenArray inputBuffer = tokenise( inputString, errorFlag );        // input buffer holds tokens to be parsed
    if ( errorFlag ) {
        return false;
    }

    // Parse tokens
    Stack stateStack;                                        // holds states during parsing
    int sdtResultBuffer[10];                                 // holds syntax directed translation results
    Token lookahead;                                         // active token in the input buffer
    Rule act;                                                // holds table entry correspoding to the table[ topOfStack ][ lookaheadChar ]
    int  i, k = 0;                                           // i: input buffer lookahead index;    k: Syntax Directed Translation buffer index

    while( i < inputBuffer.size ) {
        lookahead = inputBuffer.tokenArr[i];
        act = parseTable[ stateStack.top() ][ lookahead.identity - tDOLLAR ];       // (inputBuffer[i] - tDOLLAR) eg. for "+", (2201 - 2200) = 1st column

        if ( act >= Rule::STATE00 && act < Rule::STATEMAX ) {                       // Shift
            shift( act, lookahead, stateStack, i, k, sdtResultBuffer );

        } else if ( act > Rule::RULEMAX && act <= Rule::RULE01 ) {                  // Reduce A -> expression
            reduce( stateStack, sdtResultBuffer, k, act );

        } else if ( act > Rule::ERRORMAX && act <= Rule::ERROR00 ) {                // Error detected
            errorFlag = true;
            errorHandler( act, lookahead, stateStack, i, k, sdtResultBuffer, inputBuffer.size );

        } else if ( act == Rule::ACCEPT ) {                 // Successfully parsed
            if ( errorFlag ) {
                cout << "Parsed with error: " << sdtResultBuffer[0] << endl;
            } else {
                cout << "SUCCESS: " << sdtResultBuffer[0] << endl;
            } return true;

        } else {
            // Error somewhere in the parse table or rules. Should never execute this.
            cout << "PARSER MALFUNCTION! Unknown code: "  << act
                 << " returned by action().";
            return false;
        }
    }
    cout << "FAILED";
    return false;
}

void errorHandler( const Rule errorCode, const Token lookahead, Stack stateStack, int& i, int& k, int sdtResultBuffer[], int numOfTokens ) {
    bool isReducing = true;
    Rule ruleToUse;
    cout << "ERROR: ";
    switch ( errorCode ) {
        case ERROR00:
            cout << "Unexpected end of file\n"; i = numOfTokens; return;

        case ERROR02:
            // maybe check string size to see if one extra char given, or expecting plus
            if ( i == numOfTokens-1 ) {
                cout << "2 -- Extra character at the end of input " << getLookaheadChar( lookahead.identity ) << endl;
                i++;
            } else {
                cout << "2-- Expected '+'. Got " << getLookaheadChar( lookahead.identity ) << endl;
                ruleToUse = STATE06; isReducing = false;
            }
            break;

        case ERROR01:
            cout << "1 -- Missing operand. Got " << getLookaheadChar( lookahead.identity ) << endl;
            ruleToUse = STATE05; isReducing = false; break;
        case ERROR07:
            cout << "7 -- Expected operand. Got " << getLookaheadChar( lookahead.identity ) << endl;
            ruleToUse = STATE11; isReducing = false; break;

        case ERROR03: cout << "3 -- "; ruleToUse = RULE02; break;
        case ERROR04: cout << "4 -- "; ruleToUse = RULE04; break;
        case ERROR06: cout << "6 -- "; ruleToUse = RULE06; break;
        case ERROR08: cout << "8 -- "; ruleToUse = RULE01; break;
        case ERROR09: cout << "9 -- "; ruleToUse = RULE03; break;
        case ERROR10: cout << "10-- "; ruleToUse = RULE05; break;
    } cout << flush;

    if ( isReducing ) {
        cout << "Expected operator. Got " << getLookaheadChar( lookahead.identity ) << endl;
        reduce( stateStack, sdtResultBuffer, k, ruleToUse ); i++;
    } else {
        shift( ruleToUse, lookahead, stateStack, i, k, sdtResultBuffer );
    }
}

string getLookaheadChar( Rule lookahead ) {
    string found;
    switch ( lookahead ) {
        case tSTAR:     found = "*";    break;
        case tPLUS:     found = "+";    break;
        case tNUM:      found = "num";  break;
        case tOPENPAR:  found = "(";    break;
        case tCLOSEPAR: found = ")";    break;
        case tDOLLAR:   found = "$";    break;
    } return found;
}

void shift( Rule act, Token lookahead, Stack& stateStack, int& i, int& k, int sdtResultBuffer[] ) {
    stateStack.push( act );
    if ( lookahead.identity == tNUM ) {
        sdtResultBuffer[ k++ ] = lookahead.tokenVal;
    } i++;
}

void reduce( Stack& stateStack, int sdtResultBuffer[], int& k, Rule derivationRule ) {
    Rule nonTerm;
    switch( derivationRule ) {
        case RULE01:
            nonTerm = vE;
            stateStack.pop(3);
            sdtResultBuffer[k-2] += sdtResultBuffer[k-1]; k--;
            break;
        case RULE02:
            nonTerm = vE;
            stateStack.pop();
            break;
        case RULE03:
            nonTerm = vT;
            stateStack.pop(3);
            sdtResultBuffer[k-2] *= sdtResultBuffer[k-1]; k--;
            break;
        case RULE04:
            nonTerm = vT;
            stateStack.pop();
            break;
        case RULE05:
            nonTerm = vM;
            stateStack.pop(3);
            break;
        case RULE06:
            nonTerm = vM;
            stateStack.pop();
            break;
        default: throw "PARSER MALFUNCTION: UNKNOWN RULE GIVEN TO REDUCE";
    }
    stateStack.push( parseTable[ stateStack.top() ][ nonTerm - tDOLLAR ] );
}

TokenArray tokenise( string inputString, bool& error ) {
    const int ASCII_DISPLACEMENT = 48;
    int digitBuffer[8];
    int digitCount = 0;
    TokenArray tokens;

    for ( int i = 0; i < inputString.size(); i++ ) {

        if ( '0' <= inputString[i]  && inputString[i] <= '9' ) {                        // if current is a digit
            digitBuffer[ digitCount++ ] = ( inputString[i] - ASCII_DISPLACEMENT );

            if ( inputString[i+1] < '0' || inputString[i+1] > '9' ) {                 // if next is not a digit
                tokens.emplace_back( parseDigitVal( digitBuffer, digitCount ) );
                digitCount = 0;
            }                                                               // ensures multi-digit numbers are counted as a single num

        } else {
            switch( inputString[i] ) {
                case '+': tokens.emplace_back( tPLUS );      continue;
                case '*': tokens.emplace_back( tSTAR );      continue;
                case '(': tokens.emplace_back( tOPENPAR );   continue;
                case ')': tokens.emplace_back( tCLOSEPAR );  continue;
                default : printInputError( inputString, i ); error = true;        // print the string with the error highlighted
            }
        }
    } tokens.emplace_back( tDOLLAR );
    return tokens;
} // converts char to tokens the parser understands

void printInputError( const string inputString, int position ) {
    int i;
    cout << "UNKNOWN CHAR AT input[" << position << "]: ";
    for ( i = 0; i < position; i++ ) {
        cout << inputString[i];
    } cout << " |^" << inputString[ position ] << "^| ";
    for ( i = position + 1; i < inputString.size(); i++ ) {
        cout << inputString[i];
    } cout << endl;
}
int parseDigitVal( int digits[8], int digitCount ) {
    int value = 0;
    int power = digitCount;
    for ( int i = 0; i < digitCount; i++ ) {
        if ( digits[i] != 0 ) {
            value += ( digits[i] * power10( --power ) );
        } else {
            value *= power10( --power );
        }
    }
    return value;
}
int power10( unsigned int y ) {     /* calculates 10 ^ y */
    int temp;

    if( y == 0 ) return 1;

    temp = power10( y / 2 );

    if ( y % 2 == 0 )
        return temp * temp;
    else
        return 10 * temp * temp;
}

/*
void printInput( Token* inputBuff) {
    int i = 0;
    while ( inputBuff[i].identity != tDOLLAR ) {
        cout << inputBuff[i++].tokenVal << " ";
    } cout << endl;
}

void printProductionReduced( Rule act ) {
    string rule;
    switch ( act ){
        case RULE01: rule = "E → E + T";   break;
        case RULE02: rule = "E → T";       break;
        case RULE03: rule = "T → T * M";   break;
        case RULE04: rule = "T → M";       break;
        case RULE05: rule = "M → ( E )";   break;
        case RULE06: rule = "M → num";     break;
        default: rule = "PRINTER ERROR: UNKNOWN RULE SENT TO PRINT FUNCTION";
    } cout << "\t" << rule << endl;
}
*/
