#include "parser.h"

using namespace std;

/*
 *	Block Element callbacks
 */

static void rndr_blockcode(struct buf *ob, struct buf *text, void *opaque);
static void rndr_blockquote(struct buf *ob, struct buf *text, void *opaque);
static void rndr_header(struct buf *ob, struct buf *text, int level, void *opaque);
static void rndr_list(struct buf *ob, struct buf *text, int flags, void *opaque);
static void rndr_listitem(struct buf *ob, struct buf *text, int flags, void *opaque);
static void rndr_paragraph(struct buf *ob, struct buf *text, void *opaque);

/*
 *	Span Element callbacks
 */

static int rndr_codespan(struct buf *ob, struct buf *text, void *opaque);
static int rndr_double_emphasis(struct buf *ob, struct buf *text, char c, void *opaque);
static int rndr_emphasis(struct buf *ob, struct buf *text, char c, void *opaque);
static int rndr_triple_emphasis(struct buf *ob, struct buf *text, char c, void *opaque);
static int rndr_linebreak(struct buf *ob, void *opaque);
static int rndr_link(struct buf *ob, struct buf *link, struct buf *title, struct buf *content, void *opaque);

/*
 *	Low Level Callbacks
 */

static void rndr_normal_text(struct buf *ob, struct buf *text, void *opaque);

struct mkd_renderer mkd_callbacks = {
	/* document-level callbacks */
	NULL, // prolog
	NULL, // epilogue

	/* block-level callbacks */
	rndr_blockcode, // BlockCode
	rndr_blockquote, // blockQuote
	NULL, // block html
	rndr_header, // header
	NULL, // hrule
	rndr_list, // list
	rndr_listitem, // listitem
	rndr_paragraph, // paragraph
	NULL, // table
	NULL, // table cell
	NULL, // table row

	/* span-level callbacks */
	NULL, // autolink
	rndr_codespan, // codespan
	rndr_double_emphasis, // double_emphasis
	rndr_emphasis, // emphasis
	NULL, // image
	rndr_linebreak, // line break
	rndr_link, // link
	NULL, // raw html tag
	rndr_triple_emphasis, // triple emphasis

	/* low-level callbacks */
	NULL, // entity
	rndr_normal_text, // normal text

	/* renderer data */
	64, // max stack
	"*_",
	NULL // opaque
};

namespace Bypass {

	Parser::Parser() {

	}

	Parser::~Parser() {
		while(!tempSpanElements.empty()) {
			delete tempSpanElements.back();
			tempSpanElements.pop_back();
		}

		if (tempBlockElement != NULL) {
			delete tempBlockElement;
		}
	}

	Document
	Parser::parse(const char *str) {
		Document result;
		this->document = &result;

		if (str) {
			struct buf *ib, *ob;

			ib = bufnew(INPUT_UNIT);
			bufputs(ib, str);

			ob = bufnew(OUTPUT_UNIT);

			mkd_callbacks.opaque = this;
			//parse and assemble document
			markdown(ob, ib, &mkd_callbacks);

			// We have finished parsing, move any data left in the temp string to the main string
			if (tempBlockElement != NULL) {
				moveTempToDocument();
			}

			/* cleanup */
			bufrelease(ib);
			bufrelease(ob);
		}

		this->document = NULL;
		return result;
	}

	Document Parser::parse(const std::string &str) {
		return parse(str.c_str());
	}

	void Parser::moveTempToDocument() {
		if (tempBlockElement != NULL) {
			this->document->append(*tempBlockElement);
			delete tempBlockElement;
			tempBlockElement = NULL;
		}
	}

	void Parser::stackTempElement(BlockElement blockElement) {
		BlockElement* element = new BlockElement(blockElement);
		if (tempBlockElement != NULL) {
			element->append(*tempBlockElement);
			delete tempBlockElement;
		}
		this->tempBlockElement = element;
	}

	void Parser::clearSpanElements() {
		tempSpanElements.clear();
	}

	// Block Element Callbacks

	void Parser::parsedBlockcode(struct buf *ob, struct buf *text) {

	}

	void Parser::parsedBlockquote(struct buf *ob, struct buf *text) {

	}

	void Parser::parsedHeader(struct buf *ob, struct buf *text, int level) {
		//check if the text contains a hash from the last block level
		//if it exists make a new container level and stick the old temp string into it

		//if there is no hash
		if (ob->size == 0) {
			moveTempToDocument();
		}

		Bypass::BlockElement element;
		element.setText(text->data);
		stackTempElement(element);
	}

	void Parser::parsedList(struct buf *ob, struct buf *text, int flags) {

	}

	void Parser::parsedListItem(struct buf *ob, struct buf *text, int flags) {

	}

	void Parser::parsedParagraph(struct buf *ob, struct buf *text) {
		//check if the text contains a hash from the last block level
		//if it exists make a new container level and stick the old temp string into it

		//if there is no hash
		if (ob->size == 0) {
			moveTempToDocument();
		}

		Bypass::BlockElement element;
		if (text->size) {
			element.setText(text->data);
		}
		element.setSpanElements(tempSpanElements);
		clearSpanElements();
		stackTempElement(element);
	}

	// Span Element Callbacks

	int Parser::parsedCodeSpan(struct buf *ob, struct buf *text) {
		return 0;
	}

	int Parser::parsedDoubleEmphasis(struct buf *ob, struct buf *text, char c) {
		SpanElement* element = new SpanElement();
		element->setText(text->data);
		element->setType("bold");
		tempSpanElements.push_back(element);

		return 1;
	}

	int Parser::parsedEmphasis(struct buf *ob, struct buf *text, char c) {
		SpanElement* element = new SpanElement();
		element->setText(text->data);
		element->setType("italic");
		tempSpanElements.push_back(element);

		return 1;
	}

	int Parser::parsedTripleEmphasis(struct buf *ob, struct buf *text, char c) {
		SpanElement* element = new SpanElement();
		element->setText(text->data);
		element->setType("bolditalic");
		tempSpanElements.push_back(element);

		return 1;
	}

	int Parser::parsedLinebreak(struct buf *ob) {
		return 0;
	}

	int Parser::parsedLink(struct buf *ob, struct buf *link, struct buf *title, struct buf *content) {
		SpanElement* element = new SpanElement();
		element->setText(content->data);
		element->setExtra(link->data);
		element->setType("href");
		tempSpanElements.push_back(element);

		return 1;
	}

	// Low Level Callbacks

	void Parser::parsedNormalText(struct buf *ob, struct buf *text) {
		SpanElement* element = new SpanElement();
		element->setText(text->data);
		element->setType("text");
		tempSpanElements.push_back(element);
	}

}

/*
 *	Block Element callbacks
 */

static void rndr_blockcode(struct buf *ob, struct buf *text, void *opaque) {
	((Bypass::Parser*) opaque)->parsedBlockcode(ob, text);
}

static void rndr_blockquote(struct buf *ob, struct buf *text, void *opaque) {
	((Bypass::Parser*) opaque)->parsedBlockquote(ob, text);
}

static void rndr_header(struct buf *ob, struct buf *text, int level, void *opaque) {
	((Bypass::Parser*) opaque)->parsedHeader(ob, text, level);
}

static void rndr_list(struct buf *ob, struct buf *text, int flags, void *opaque) {
	((Bypass::Parser*) opaque)->parsedList(ob, text, flags);
}

static void rndr_listitem(struct buf *ob, struct buf *text, int flags, void *opaque) {
	((Bypass::Parser*) opaque)->parsedListItem(ob, text, flags);
}

static void rndr_paragraph(struct buf *ob, struct buf *text, void *opaque) {
	((Bypass::Parser*) opaque)->parsedParagraph(ob, text);
}

/*
 *	Span Element callbacks
 */

static int rndr_codespan(struct buf *ob, struct buf *text, void *opaque) {
	return ((Bypass::Parser*) opaque)->parsedCodeSpan(ob, text);
}

static int rndr_double_emphasis(struct buf *ob, struct buf *text, char c, void *opaque) {
	return ((Bypass::Parser*) opaque)->parsedDoubleEmphasis(ob, text, c);
}

static int rndr_emphasis(struct buf *ob, struct buf *text, char c, void *opaque) {
	return ((Bypass::Parser*) opaque)->parsedEmphasis(ob, text, c);
}

static int rndr_triple_emphasis(struct buf *ob, struct buf *text, char c, void *opaque) {
	return ((Bypass::Parser*) opaque)->parsedTripleEmphasis(ob, text, c);
}

static int rndr_linebreak(struct buf *ob, void *opaque) {
	return ((Bypass::Parser*) opaque)->parsedLinebreak(ob);
}

static int rndr_link(struct buf *ob, struct buf *link, struct buf *title, struct buf *content, void *opaque) {
	return ((Bypass::Parser*) opaque)->parsedLink(ob, link, title, content);
}

/*
 *	Low Level Callbacks
 */

static void rndr_normal_text(struct buf *ob, struct buf *text, void *opaque) {
	return ((Bypass::Parser*) opaque)->parsedNormalText(ob, text);
}

