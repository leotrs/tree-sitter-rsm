import XCTest
import SwiftTreeSitter
import TreeSitterRsm

final class TreeSitterRsmTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_rsm())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Rsm grammar")
    }
}
