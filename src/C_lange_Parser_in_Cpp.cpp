#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept> // Required for std::runtime_error

using namespace std;

// --- DATA STRUCTURES ---

class Token {
public:
    string token_value;
    string token_class;
    int line_number;
};

struct ParseNode {
    string type;
    string value;
    int line;
    vector<ParseNode*> children;
    ~ParseNode() {
        for (ParseNode* child : children) {
            delete child;
        }
    }
};

// --- THE PARSER CLASS ---

class Parser {
public:
    Parser(const vector<Token>& tokens) : m_tokens(tokens) {}

    ParseNode* parse() {
        try {
            return parse_program();
        } catch (const runtime_error& e) {
            return nullptr;
        }
    }

private:
    const vector<Token>& m_tokens;
    size_t m_current_pos = 0;

    // ===================================================================
    // ===       UTILITY METHODS (REVISED FOR CORRECTNESS)           ===
    // ===================================================================
    // The previous versions of these functions had several logical bugs that
    // could cause infinite loops or segmentation faults. This new design is
    // simpler, safer, and correct.

    // **FIXED**: This is the simplest, most fundamental check. It must be
    // independent and not call any other parser methods.
    bool is_at_end() {
        return m_current_pos >= m_tokens.size();
    }

    // **FIXED**: This function's only job is to move the main cursor forward
    // until it points to a meaningful (non-comment) token.
    void skip_comments() {
        while (!is_at_end() &&
               (m_tokens[m_current_pos].token_class == "Single-Line Comment" ||
                m_tokens[m_current_pos].token_class == "Multi-Line Comment")) {
            m_current_pos++;
        }
    }

    // **FIXED**: `peek` is now much simpler. It ensures comments are skipped
    // and then safely returns the current token. The complex lookahead logic
    // has been moved into the functions that actually need it.
    const Token& peek() {
        skip_comments(); // ALWAYS ensure we are on a meaningful token before peeking.
        if (is_at_end()) {
            static Token eof_token = {"", "EOF", -1}; // A safe, static EOF token.
            return eof_token;
        }
        return m_tokens[m_current_pos];
    }
    
    // **NEW**: A dedicated lookahead function for the one case where we need it.
    // This is much cleaner than complicating the main `peek` function.
    const Token& lookahead(int offset) {
        skip_comments(); // Start from the current meaningful token.
        size_t lookahead_pos = m_current_pos;
        while (offset > 0 && lookahead_pos < m_tokens.size()) {
            lookahead_pos++;
            // Skip comments at the lookahead position.
            while (lookahead_pos < m_tokens.size() &&
                   (m_tokens[lookahead_pos].token_class == "Single-Line Comment" ||
                    m_tokens[lookahead_pos].token_class == "Multi-Line Comment")) {
                lookahead_pos++;
            }
            offset--;
        }

        if (lookahead_pos >= m_tokens.size()) {
            static Token eof_token = {"", "EOF", -1};
            return eof_token;
        }
        return m_tokens[lookahead_pos];
    }


    // **FIXED**: `advance` should only ever do one thing: move the cursor.
    // The next call to `peek()` will handle any comments that follow.
    void advance() {
        if (!is_at_end()) {
            m_current_pos++;
        }
    }

    // `match` remains the same, but it's now supported by the corrected helpers.
    Token match(const string& expected_class, const string& expected_value = "") {
        const Token& token = peek();
        if (token.token_class == expected_class && (expected_value.empty() || token.token_value == expected_value)) {
            Token matched_token = token;
            advance();
            return matched_token;
        }
        string error_message = "Expected " + expected_class;
        if (!expected_value.empty()) error_message += " with value '" + expected_value + "'";
        error_message += ", but got " + token.token_class + " with value '" + token.token_value + "'";
        report_error(error_message);
        throw runtime_error("Syntax Error");
    }

    // --- ERROR REPORTING ---
    void report_error(const string& message) {
        if (is_at_end()) {
            cerr << "[End of File] Syntax Error: " << message << endl;
        } else {
            cerr << "[Line " << peek().line_number << "] Syntax Error: " << message << endl;
        }
    }

    // --- RECURSIVE DESCENT PARSING FUNCTIONS ---

    // **FIXED**: Removed the stray `advance()` call that was eating the first token.
    ParseNode* parse_program() {
        ParseNode* program_node = new ParseNode{"Program", "", (m_tokens.empty() ? 0 : peek().line_number)};
        while (!is_at_end()) {
            program_node->children.push_back(parse_top_level_declaration());
        }
        cout << "Parsing completed successfully." << endl;
        return program_node;
    }

    // **FIXED**: Now uses the new, safer `lookahead()` function.
    ParseNode* parse_top_level_declaration() {
        if (peek().token_class == "PREPROCESSOR DIRECTIVE") {
            Token directive = match("PREPROCESSOR DIRECTIVE");
            return new ParseNode{"PreprocessorDirective", directive.token_value, directive.line_number};
        }
        if (peek().token_class == "KEYWORD" &&
            (peek().token_value == "int" || peek().token_value == "float" ||
             peek().token_value == "char" || peek().token_value == "void" || peek().token_value == "const")) {
            
            // Look at the token AFTER the identifier to resolve ambiguity.
            // A type is token 0, an identifier is token 1. We need to see token 2.
            const Token& future_token = lookahead(2);

            if (future_token.token_value == "(") {
                return parse_function_or_prototype();
            } else {
                return parse_variable_declaration();
            }
        }
        report_error("Unrecognized top-level statement. Expected a global variable or function.");
        throw runtime_error("Syntax Error");
    }

    // The rest of the parsing functions are correct and do not need changes.
    // I am including them here for completeness of the class.

    ParseNode* parse_function_or_prototype() {
        int start_line = peek().line_number;
        Token type_token = match("KEYWORD");
        Token name_token = match("IDENTIFIER");
        match("SPECIAL CHARACTER", "(");
        // We can add parameter parsing here later
        match("SPECIAL CHARACTER", ")");
        if (peek().token_value == "{") {
            ParseNode* func_def_node = new ParseNode{"FunctionDefinition", name_token.token_value, start_line};
            func_def_node->children.push_back(new ParseNode{"TypeSpecifier", type_token.token_value, type_token.line_number});
            func_def_node->children.push_back(parse_block_statement());
            return func_def_node;
        } else if (peek().token_value == ";") {
            match("SPECIAL CHARACTER", ";");
            ParseNode* func_proto_node = new ParseNode{"FunctionPrototype", name_token.token_value, start_line};
            func_proto_node->children.push_back(new ParseNode{"TypeSpecifier", type_token.token_value, type_token.line_number});
            return func_proto_node;
        } else {
            report_error("Expected '{' for function body or ';' for prototype after function signature.");
            throw runtime_error("Syntax Error");
        }
    }

    ParseNode* parse_variable_declaration() {
        int start_line = peek().line_number;
        ParseNode* decl_statement_node = new ParseNode{"VariableDeclarationStatement", "", start_line};
        if (peek().token_value == "const") {
            Token t = match("KEYWORD", "const");
            decl_statement_node->children.push_back(new ParseNode{"Keyword", t.token_value, t.line_number});
        }
        Token type_token = match("KEYWORD");
        decl_statement_node->children.push_back(new ParseNode{"TypeSpecifier", type_token.token_value, type_token.line_number});
        do {
            if (peek().token_value == ",") {
                match("SPECIAL CHARACTER", ",");
            }
            Token var_token = match("IDENTIFIER");
            ParseNode* declarator_node = new ParseNode{"Declarator", var_token.token_value, var_token.line_number};
            if (peek().token_value == "=") {
                match("OPERATOR", "=");
                ParseNode* initializer_node = new ParseNode{"Initializer", "=", peek().line_number};
                initializer_node->children.push_back(parse_expression());
                declarator_node->children.push_back(initializer_node);
            }
            decl_statement_node->children.push_back(declarator_node);
        } while (peek().token_value == ",");
        match("SPECIAL CHARACTER", ";");
        return decl_statement_node;
    }

    ParseNode* parse_statement() {
        const string& token_value = peek().token_value;
        if (token_value == "if") return parse_if_statement();
        if (token_value == "for") return parse_for_statement();
        if (token_value == "return") return parse_return_statement();
        if (token_value == "{") return parse_block_statement();
        if (token_value == ";") {
            int line = peek().line_number;
            match("SPECIAL CHARACTER", ";");
            return new ParseNode{"EmptyStatement", ";", line};
        }
        if (token_value == "const" || token_value == "int" ||
            token_value == "float" || token_value == "char") {
            return parse_variable_declaration();
        }
        return parse_expression_statement();
    }

    ParseNode* parse_block_statement() {
        int start_line = peek().line_number;
        match("SPECIAL CHARACTER", "{");
        ParseNode* block_node = new ParseNode{"BlockStatement", "{}", start_line};
        while (peek().token_value != "}") {
            block_node->children.push_back(parse_statement());
        }
        match("SPECIAL CHARACTER", "}");
        return block_node;
    }

    ParseNode* parse_if_statement() {
        int start_line = peek().line_number;
        match("KEYWORD", "if");
        ParseNode* if_node = new ParseNode{"IfStatement", "if", start_line};
        match("SPECIAL CHARACTER", "(");
        if_node->children.push_back(parse_expression());
        match("SPECIAL CHARACTER", ")");
        if_node->children.push_back(parse_statement());
        if (peek().token_value == "else") {
            match("KEYWORD", "else");
            if_node->children.push_back(parse_statement());
        }
        return if_node;
    }

    ParseNode* parse_return_statement() {
        int start_line = peek().line_number;
        match("KEYWORD", "return");
        ParseNode* return_node = new ParseNode{"ReturnStatement", "return", start_line};
        if (peek().token_value != ";") {
            return_node->children.push_back(parse_expression());
        }
        match("SPECIAL CHARACTER", ";");
        return return_node;
    }

    ParseNode* parse_expression_statement() {
        int start_line = peek().line_number;
        ParseNode* expr_stmt_node = new ParseNode{"ExpressionStatement", "", start_line};
        expr_stmt_node->children.push_back(parse_expression());
        match("SPECIAL CHARACTER", ";");
        return expr_stmt_node;
    }
/*-------------
    ParseNode* parse_for_statement() {
        int start_line = peek().line_number;
        match("KEYWORD", "for");
        ParseNode* for_node = new ParseNode{"ForStatement", "for", start_line};
        match("SPECIAL CHARACTER", "(");
        if (peek().token_value == ";") {
            match("SPECIAL CHARACTER", ";");
            for_node->children.push_back(new ParseNode{"Empty", "initializer", start_line});
        } else if (peek().token_value == "int" || peek().token_value == "char" || peek().token_value == "float") {
            for_node->children.push_back(parse_variable_declaration());
        } else {
            for_node->children.push_back(parse_expression_statement());
        }
        if (peek().token_value == ";") {
            match("SPECIAL CHARACTER", ";");
            for_node->children.push_back(new ParseNode{"Empty", "condition", start_line});
        } else {
            for_node->children.push_back(parse_expression());
            match("SPECIAL CHARACTER", ";");
        }
        if (peek().token_value == ")") {
            for_node->children.push_back(new ParseNode{"Empty", "increment", start_line});
        } else {
            for_node->children.push_back(parse_expression());
        }
        match("SPECIAL CHARACTER", ")");
        for_node->children.push_back(parse_statement());
        return for_node;
    }
----------------*/
// REPLACE your old parse_for_statement() with this new, cleaner version.

// Rule: for_statement -> 'for' '(' initializer condition increment ')' statement
ParseNode* parse_for_statement() {
    int start_line = peek().line_number;
    match("KEYWORD", "for");
    ParseNode* for_node = new ParseNode{"ForStatement", "for", start_line};
    
    match("SPECIAL CHARACTER", "(");

    // --- 1. Parse Initializer ---
    // This part can remain the same. It correctly handles the three cases.
    if (peek().token_value == ";") {
        match("SPECIAL CHARACTER", ";");
        for_node->children.push_back(new ParseNode{"Empty", "initializer", start_line});
    } else if (peek().token_value == "int" || peek().token_value == "char" || peek().token_value == "float") {
        for_node->children.push_back(parse_variable_declaration());
    } else {
        for_node->children.push_back(parse_expression_statement());
    }

    // --- 2. Parse Condition (REVISED) ---
    // If the condition is not empty, parse the expression and add it DIRECTLY.
    if (peek().token_value == ";") {
        match("SPECIAL CHARACTER", ";");
        for_node->children.push_back(new ParseNode{"Empty", "condition", start_line});
    } else {
        // THE FIX: No extra "Condition" wrapper node is created.
        for_node->children.push_back(parse_expression());
        match("SPECIAL CHARACTER", ";");
    }

    // --- 3. Parse Increment (REVISED) ---
    // If the increment is not empty, parse the expression and add it DIRECTLY.
    if (peek().token_value == ")") {
        // Empty increment
        for_node->children.push_back(new ParseNode{"Empty", "increment", start_line});
    } else {
        // THE FIX: No extra "UPDATE" or "Increment" wrapper node is created.
        for_node->children.push_back(parse_expression());
    }

    match("SPECIAL CHARACTER", ")");
    
    // --- 4. Parse the Body Statement ---
    // This part remains the same.
    for_node->children.push_back(parse_statement());

    return for_node;
}
    ParseNode* parse_expression() { return parse_assignment(); }
    ParseNode* parse_assignment() {
        int start_line = peek().line_number;
        ParseNode* left_node = parse_equality();
        if (peek().token_value == "=") {
            Token op = match("OPERATOR", "=");
            ParseNode* right_node = parse_assignment();
            ParseNode* assignment_node = new ParseNode{"AssignmentExpression", op.token_value, start_line};
            assignment_node->children.push_back(left_node);
            assignment_node->children.push_back(right_node);
            return assignment_node;
        }
        return left_node;
    }
    ParseNode* parse_equality() {
        ParseNode* left_node = parse_relational();
        while (peek().token_value == "==" || peek().token_value == "!=") {
            Token op = match("OPERATOR");
            ParseNode* right_node = parse_relational();
            ParseNode* new_left = new ParseNode{"BinaryExpression", op.token_value, op.line_number};
            new_left->children.push_back(left_node);
            new_left->children.push_back(right_node);
            left_node = new_left;
        }
        return left_node;
    }
    ParseNode* parse_relational() {
        ParseNode* left_node = parse_additive();
        while (peek().token_value == "<" || peek().token_value == ">" ||
               peek().token_value == "<=" || peek().token_value == ">=") {
            Token op = match("OPERATOR");
            ParseNode* right_node = parse_additive();
            ParseNode* new_left = new ParseNode{"BinaryExpression", op.token_value, op.line_number};
            new_left->children.push_back(left_node);
            new_left->children.push_back(right_node);
            left_node = new_left;
        }
        return left_node;
    }
    ParseNode* parse_additive() {
        ParseNode* left_node = parse_multiplicative();
        while (peek().token_value == "+" || peek().token_value == "-") {
            Token op = match("OPERATOR");
            ParseNode* right_node = parse_multiplicative();
            ParseNode* new_left = new ParseNode{"BinaryExpression", op.token_value, op.line_number};
            new_left->children.push_back(left_node);
            new_left->children.push_back(right_node);
            left_node = new_left;
        }
        return left_node;
    }
    ParseNode* parse_multiplicative() {
        ParseNode* left_node = parse_primary();
        while (peek().token_value == "*" || peek().token_value == "/") {
            Token op = match("OPERATOR");
            ParseNode* right_node = parse_primary();
            ParseNode* new_left = new ParseNode{"BinaryExpression", op.token_value, op.line_number};
            new_left->children.push_back(left_node);
            new_left->children.push_back(right_node);
            left_node = new_left;
        }
        return left_node;
    }
    ParseNode* parse_primary() {
        int line = peek().line_number;
        if (peek().token_class == "NUMERIC CONSTANT") {
            Token value = match("NUMERIC CONSTANT");
            return new ParseNode{"Constant", value.token_value, line};
        }
        if (peek().token_class == "IDENTIFIER") {
            Token value = match("IDENTIFIER");
            return new ParseNode{"Identifier", value.token_value, line};
        }
        if (peek().token_value == "(") {
            match("SPECIAL CHARACTER", "(");
            ParseNode* expr_node = parse_expression();
            match("SPECIAL CHARACTER", ")");
            return expr_node;
        }
        report_error("Expected a value, variable, or expression in parentheses.");
        throw runtime_error("Syntax Error");
    }
};

// --- FILE READING LOGIC ---

vector<Token> load_tokens_from_file(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Fatal Error: Could not open token file '" << filename << "'" << endl;
        return {};
    }

    vector<Token> loaded_tokens;
    string line;
    while (getline(file, line)) {
        if (line.length() < 5) continue;

        size_t first_comma = line.find(',');
        size_t last_comma = line.rfind(',');

        if (first_comma == string::npos || last_comma == string::npos || first_comma == last_comma) {
            cerr << "Warning: Malformed token line, skipping: " << line << endl;
            continue;
        }

        // **FIXED**: The length of the final part needs to account for the trailing '>'.
        string token_class = line.substr(1, first_comma - 1);
        string token_value = line.substr(first_comma + 2, last_comma - (first_comma + 2));
        string line_str = line.substr(last_comma + 2, line.length() - (last_comma + 2) - 1);

        Token t;
        t.token_class = token_class;
        t.token_value = token_value;
        try {
            t.line_number = stoi(line_str);
        } catch (...) {
            cerr << "Warning: Malformed line number '" << line_str << "', skipping line: " << line << endl;
            continue;
        }
        loaded_tokens.push_back(t);
    }
    cout << "Token file loaded. " << loaded_tokens.size() << " tokens read." << endl;
    return loaded_tokens;
}
/*----------------------------
// --- PARSE TREE VISUALIZATION V1---

void print_node(const ParseNode* node, const string& prefix, bool is_last) {
    if (!node) return;
    cout << prefix << (is_last ? "└── " : "├── ") << node->type << " (" << node->value << ")" << " [Line: " << node->line << "]" << endl;
    string child_prefix = prefix + (is_last ? "    " : "│   ");
    for (size_t i = 0; i < node->children.size(); ++i) {
        print_node(node->children[i], child_prefix, i == node->children.size() - 1);
    }
}

void visualize_parse_tree(const ParseNode* root) {
    if (!root) {
        cout << "Parse tree is empty." << endl;
        return;
    }
    cout << "--- Abstract Syntax Tree ---" << endl;
    print_node(root, "", true);
    cout << "--------------------------" << endl;
}
------------------------*/
// ===================================================================
// ===         PARSE TREE VISUALIZATION (CORRECTED)              ===
// ===================================================================

// This is the recursive helper function that does the actual printing.
void print_node(const ParseNode* node, const string& prefix, bool is_last_sibling) {
    if (!node) return;

    // 1. Print the prefix for the current node's line.
    // This part correctly uses "└──" for the last sibling and "├──" for others.
    cout << prefix << (is_last_sibling ? "└── " : "├── ");

    // 2. Print the node's own information.
    cout << node->type << " (" << node->value << ")" << " [Line: " << node->line << "]" << endl;

    // 3. **THE CRITICAL FIX**: Prepare the prefix for the children.
    // The prefix for the children is the parent's prefix PLUS a new segment.
    // If the parent (the current node) is the last sibling, the new segment is just spaces.
    // Otherwise, it's a vertical bar to show the connection to the parent's next sibling.
    string child_prefix = prefix + (is_last_sibling ? "    " : "│   ");

    // 4. Recursively print each of the children.
    for (size_t i = 0; i < node->children.size(); ++i) {
        // The last child in the vector is the last sibling.
        bool is_last_child = (i == node->children.size() - 1);
        print_node(node->children[i], child_prefix, is_last_child);
    }
}

// This is the public-facing function to start the visualization.
void visualize_parse_tree(const ParseNode* root) {
    if (!root) {
        cout << "Parse tree is empty." << endl;
        return;
    }
    cout << "--- Abstract Syntax Tree ---" << endl;
    
    // The root node is always the "last" node at its level, so we start with true.
    // It has no prefix.
    print_node(root, "", true);

    cout << "--------------------------" << endl;
}
// --- MAIN FUNCTION ---

int main() {
    const string token_file = "tokens.txt";
    vector<Token> tokens = load_tokens_from_file(token_file);

    if (tokens.empty()) {
        cout << "No tokens to parse. Halting." << endl;
        return 1;
    }

    cout << "---------------------------------" << endl;
    cout << "Starting Parser..." << endl;
    Parser parser(tokens);
    ParseNode* parse_tree = parser.parse();

    cout << "---------------------------------" << endl;
    if (parse_tree != nullptr) {
        cout << "Program is syntactically valid." << endl;
        visualize_parse_tree(parse_tree);
        delete parse_tree;
    } else {
        cout << "Program has one or more syntax errors." << endl;
    }
    
    cout << "Press enter to end the program.";
    cin.get();
    return 0;
}