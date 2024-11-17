#include "json_common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINT_ONLY_FAILURE 1

typedef struct {
    const char *filename;
    plain_json_ErrorType expected_result;
} TestCase;

const TestCase test_parsing_cases[] = {
    /* Implementation defined */
    /* TODO: Decide how to handle surrogates */
    { "i_number_double_huge_neg_exp.json", PLAIN_JSON_DONE },
    { "i_number_huge_exp.json", PLAIN_JSON_DONE },
    { "i_number_neg_int_huge_exp.json", PLAIN_JSON_DONE },
    { "i_number_pos_double_huge_exp.json", PLAIN_JSON_DONE },
    { "i_number_real_neg_overflow.json", PLAIN_JSON_DONE },
    { "i_number_real_pos_overflow.json", PLAIN_JSON_DONE },
    { "i_number_real_underflow.json", PLAIN_JSON_DONE },
    { "i_number_too_big_neg_int.json", PLAIN_JSON_DONE },
    { "i_number_too_big_pos_int.json", PLAIN_JSON_DONE },
    { "i_number_very_big_negative_int.json", PLAIN_JSON_DONE },

    { "i_object_key_lone_2nd_surrogate.json", PLAIN_JSON_DONE },
    { "i_string_1st_surrogate_but_2nd_missing.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE },
    { "i_string_1st_valid_surrogate_2nd_invalid.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE },
    { "i_string_UTF-16LE_with_BOM.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "i_string_UTF-8_invalid_sequence.json", PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "i_string_UTF8_surrogate_U+D800.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE },

    { "i_string_incomplete_surrogate_and_escape_valid.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "i_string_incomplete_surrogate_pair.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "i_string_incomplete_surrogates_escape_valid.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "i_string_invalid_lonely_surrogate.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "i_string_invalid_surrogate.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "i_string_invalid_utf-8.json", PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "i_string_inverted_surrogates_U+1D11E.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "i_string_iso_latin_1.json", -1 },
    { "i_string_lone_second_surrogate.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "i_string_lone_utf8_continuation_byte.json", PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "i_string_not_in_unicode_range.json", PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "i_string_overlong_sequence_2_bytes.json", PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "i_string_overlong_sequence_6_bytes.json", PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "i_string_overlong_sequence_6_bytes_null.json", PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "i_string_truncated-utf-8.json", PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "i_string_utf16BE_no_BOM.json", -1 },
    { "i_string_utf16LE_no_BOM.json", -1 },
    { "i_structure_500_nested_arrays.json", PLAIN_JSON_ERROR_NESTING_TOO_DEEP },
    { "i_structure_UTF-8_BOM_empty_object.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },

    /* Must fail */
    { "n_array_1_true_without_comma.json", PLAIN_JSON_ERROR_MISSING_COMMA },
    { "n_array_a_invalid_utf8.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_colon_instead_of_comma.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_comma_after_close.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_comma_and_number.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_double_comma.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_double_extra_comma.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_extra_close.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_extra_comma.json", PLAIN_JSON_ERROR_UNEXPECTED_COMMA },
    { "n_array_incomplete.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_array_incomplete_invalid_value.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_inner_array_no_comma.json", PLAIN_JSON_ERROR_MISSING_COMMA },
    { "n_array_invalid_utf8.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_items_separated_by_semicolon.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_just_comma.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_just_minus.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_array_missing_value.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_newlines_unclosed.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_array_number_and_comma.json", PLAIN_JSON_ERROR_UNEXPECTED_COMMA },
    { "n_array_number_and_several_commas.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_spaces_vertical_tab_formfeed.json", PLAIN_JSON_ERROR_STRING_INVALID_ASCII },
    { "n_array_star_inside.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_array_unclosed.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_array_unclosed_trailing_comma.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_array_unclosed_with_new_lines.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_array_unclosed_with_object_inside.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_incomplete_false.json", PLAIN_JSON_ERROR_KEYWORD_INVALID },
    { "n_incomplete_null.json", PLAIN_JSON_ERROR_KEYWORD_INVALID },
    { "n_incomplete_true.json", PLAIN_JSON_ERROR_KEYWORD_INVALID },
    { "n_multidigit_number_then_00.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_++.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_+1.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_+Inf.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_-01.json", PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO },
    { "n_number_-1.0..json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_-2..json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_-NaN.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_.-1.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_.2e-3.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_0.1.2.json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_0.3e+.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_0.3e.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_0.e1.json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_0_capital_E+.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_0_capital_E.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_0e+.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_0e.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_1.0e+.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_1.0e-.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_1.0e.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_1_000.json", PLAIN_JSON_ERROR_MISSING_COMMA },
    { "n_number_1eE2.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_2.e+3.json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_2.e-3.json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_2.e3.json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_9.e+.json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_Inf.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_NaN.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_U+FF11_fullwidth_digit_one.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_expression.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_hex_1_digit.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_hex_2_digits.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_infinity.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_invalid+-.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_invalid-negative-real.json", PLAIN_JSON_ERROR_MISSING_COMMA },
    { "n_number_invalid-utf-8-in-bigger-int.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_invalid-utf-8-in-exponent.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_invalid-utf-8-in-int.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_minus_infinity.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_minus_sign_with_trailing_garbage.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_minus_space_1.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_neg_int_starting_with_zero.json", PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO },
    { "n_number_neg_real_without_int_part.json", PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN },
    { "n_number_neg_with_garbage_at_end.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_real_garbage_after_e.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_real_with_invalid_utf8_after_e.json", PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO },
    { "n_number_real_without_fractional_part.json", PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL },
    { "n_number_starting_with_dot.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_with_alpha.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_with_alpha_char.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_number_with_leading_zero.json", PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO },
    { "n_object_bad_value.json", PLAIN_JSON_ERROR_KEYWORD_INVALID },
    { "n_object_bracket_key.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_comma_instead_of_colon.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_double_colon.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_emoji.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_garbage_at_end.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_key_with_single_quotes.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_lone_continuation_byte_in_key_and_trailing_comma.json",
      PLAIN_JSON_ERROR_STRING_UTF8_INVALID },
    { "n_object_missing_colon.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_missing_key.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_missing_semicolon.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_missing_value.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_object_no-colon.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_object_non_string_key.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_non_string_key_but_huge_number_instead.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_repeated_null_null.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_several_trailing_commas.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_single_quote.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_trailing_comma.json", PLAIN_JSON_ERROR_UNEXPECTED_COMMA },
    { "n_object_trailing_comment.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_trailing_comment_open.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_trailing_comment_slash_open.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_trailing_comment_slash_open_incomplete.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_two_commas_in_a_row.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_unquoted_key.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_unterminated-value.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_object_with_single_string.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_object_with_trailing_garbage.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_single_space.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },

    { "n_string_1_surrogate_then_escape.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "n_string_1_surrogate_then_escape_u.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "n_string_1_surrogate_then_escape_u1.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},
    { "n_string_1_surrogate_then_escape_u1x.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE},

    { "n_string_accentuated_char_no_quotes.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_string_backslash_00.json", PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE },
    { "n_string_escape_x.json", PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE },
    { "n_string_escaped_backslash_bad.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_string_escaped_ctrl_char_tab.json", PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE },
    { "n_string_escaped_emoji.json", PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE },
    { "n_string_incomplete_escape.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_string_incomplete_escaped_character.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID },
    { "n_string_incomplete_surrogate.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE },
    { "n_string_incomplete_surrogate_escape_invalid.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE },
    { "n_string_invalid-utf-8-in-escape.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID },
    { "n_string_invalid_backslash_esc.json", PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE },
    { "n_string_invalid_unicode_escape.json", PLAIN_JSON_ERROR_STRING_UTF16_INVALID },
    { "n_string_invalid_utf8_after_escape.json", PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE },
    { "n_string_leading_uescaped_thinspace.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_string_no_quotes_with_bad_escape.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_string_single_doublequote.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_string_single_quote.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_string_single_string_no_double_quotes.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_string_start_escape_unclosed.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_string_unescaped_ctrl_char.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_string_unescaped_newline.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_string_unescaped_tab.json", PLAIN_JSON_ERROR_STRING_INVALID_ASCII },
    { "n_string_unicode_CapitalU.json", PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE },
    { "n_string_with_trailing_garbage.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_100000_opening_arrays.json", PLAIN_JSON_ERROR_NESTING_TOO_DEEP },
    { "n_structure_U+2060_word_joined.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_UTF8_BOM_no_data.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_angle_bracket_..json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_angle_bracket_null.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_array_trailing_garbage.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_array_with_extra_array_close.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_array_with_unclosed_string.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_structure_ascii-unicode-identifier.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_capitalized_True.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_close_unopened_array.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_comma_instead_of_closing_brace.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_double_array.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_end_array.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_incomplete_UTF8_BOM.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_lone-invalid-utf-8.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_lone-open-bracket.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_no_data.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_null-byte-outside-string.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_number_with_trailing_garbage.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_object_followed_by_closing_object.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_object_unclosed_no_value.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_object_with_comment.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_object_with_trailing_garbage.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_open_array_apostrophe.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_open_array_comma.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_open_array_object.json", PLAIN_JSON_ERROR_NESTING_TOO_DEEP },
    { "n_structure_open_array_open_object.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_open_array_open_string.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_structure_open_array_string.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_open_object.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_open_object_close_array.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_open_object_comma.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_open_object_open_array.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_open_object_open_string.json", PLAIN_JSON_ERROR_STRING_UNTERMINATED },
    { "n_structure_open_object_string_with_apostrophes.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_open_open.json", PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE },
    { "n_structure_single_eacute.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_single_star.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_trailing_#.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_uescaped_LF_before_string.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_unclosed_array.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_unclosed_array_partial_null.json", PLAIN_JSON_ERROR_KEYWORD_INVALID },
    { "n_structure_unclosed_array_unfinished_false.json", PLAIN_JSON_ERROR_KEYWORD_INVALID },
    { "n_structure_unclosed_array_unfinished_true.json", PLAIN_JSON_ERROR_KEYWORD_INVALID },
    { "n_structure_unclosed_object.json", PLAIN_JSON_ERROR_UNEXPECTED_EOF },
    { "n_structure_unicode-identifier.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_whitespace_U+2060_word_joiner.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },
    { "n_structure_whitespace_formfeed.json", PLAIN_JSON_ERROR_ILLEGAL_CHAR },

    /* Must succeed */
    { "y_array_arraysWithSpaces.json", PLAIN_JSON_DONE },
    { "y_array_empty-string.json", PLAIN_JSON_DONE },
    { "y_array_empty.json", PLAIN_JSON_DONE },
    { "y_array_ending_with_newline.json", PLAIN_JSON_DONE },
    { "y_array_false.json", PLAIN_JSON_DONE },
    { "y_array_heterogeneous.json", PLAIN_JSON_DONE },
    { "y_array_null.json", PLAIN_JSON_DONE },
    { "y_array_with_1_and_newline.json", PLAIN_JSON_DONE },
    { "y_array_with_leading_space.json", PLAIN_JSON_DONE },
    { "y_array_with_several_null.json", PLAIN_JSON_DONE },
    { "y_array_with_trailing_space.json", PLAIN_JSON_DONE },
    { "y_number.json", PLAIN_JSON_DONE },
    { "y_number_0e+1.json", PLAIN_JSON_DONE },
    { "y_number_0e1.json", PLAIN_JSON_DONE },
    { "y_number_after_space.json", PLAIN_JSON_DONE },
    { "y_number_double_close_to_zero.json", PLAIN_JSON_DONE },
    { "y_number_int_with_exp.json", PLAIN_JSON_DONE },
    { "y_number_minus_zero.json", PLAIN_JSON_DONE },
    { "y_number_negative_int.json", PLAIN_JSON_DONE },
    { "y_number_negative_one.json", PLAIN_JSON_DONE },
    { "y_number_negative_zero.json", PLAIN_JSON_DONE },
    { "y_number_real_capital_e.json", PLAIN_JSON_DONE },
    { "y_number_real_capital_e_neg_exp.json", PLAIN_JSON_DONE },
    { "y_number_real_capital_e_pos_exp.json", PLAIN_JSON_DONE },
    { "y_number_real_exponent.json", PLAIN_JSON_DONE },
    { "y_number_real_fraction_exponent.json", PLAIN_JSON_DONE },
    { "y_number_real_neg_exp.json", PLAIN_JSON_DONE },
    { "y_number_real_pos_exponent.json", PLAIN_JSON_DONE },
    { "y_number_simple_int.json", PLAIN_JSON_DONE },
    { "y_number_simple_real.json", PLAIN_JSON_DONE },
    { "y_object.json", PLAIN_JSON_DONE },
    { "y_object_basic.json", PLAIN_JSON_DONE },
    { "y_object_duplicated_key.json", PLAIN_JSON_DONE },
    { "y_object_duplicated_key_and_value.json", PLAIN_JSON_DONE },
    { "y_object_empty.json", PLAIN_JSON_DONE },
    { "y_object_empty_key.json", PLAIN_JSON_DONE },
    { "y_object_escaped_null_in_key.json", PLAIN_JSON_DONE },
    { "y_object_extreme_numbers.json", PLAIN_JSON_DONE },
    { "y_object_long_strings.json", PLAIN_JSON_DONE },
    { "y_object_simple.json", PLAIN_JSON_DONE },
    { "y_object_string_unicode.json", PLAIN_JSON_DONE },
    { "y_object_with_newlines.json", PLAIN_JSON_DONE },
    { "y_string_1_2_3_bytes_UTF-8_sequences.json", PLAIN_JSON_DONE },
    { "y_string_accepted_surrogate_pair.json", PLAIN_JSON_DONE },
    { "y_string_accepted_surrogate_pairs.json", PLAIN_JSON_DONE },
    { "y_string_allowed_escapes.json", PLAIN_JSON_DONE },
    { "y_string_backslash_and_u_escaped_zero.json", PLAIN_JSON_DONE },
    { "y_string_backslash_doublequotes.json", PLAIN_JSON_DONE },
    { "y_string_comments.json", PLAIN_JSON_DONE },
    { "y_string_double_escape_a.json", PLAIN_JSON_DONE },
    { "y_string_double_escape_n.json", PLAIN_JSON_DONE },
    { "y_string_escaped_control_character.json", PLAIN_JSON_DONE },
    { "y_string_escaped_noncharacter.json", PLAIN_JSON_DONE },
    { "y_string_in_array.json", PLAIN_JSON_DONE },
    { "y_string_in_array_with_leading_space.json", PLAIN_JSON_DONE },
    { "y_string_last_surrogates_1_and_2.json", PLAIN_JSON_DONE },
    { "y_string_nbsp_uescaped.json", PLAIN_JSON_DONE },
    { "y_string_nonCharacterInUTF-8_U+10FFFF.json", PLAIN_JSON_DONE },
    { "y_string_nonCharacterInUTF-8_U+FFFF.json", PLAIN_JSON_DONE },
    { "y_string_null_escape.json", PLAIN_JSON_DONE },
    { "y_string_one-byte-utf-8.json", PLAIN_JSON_DONE },
    { "y_string_pi.json", PLAIN_JSON_DONE },
    { "y_string_reservedCharacterInUTF-8_U+1BFFF.json", PLAIN_JSON_DONE },
    { "y_string_simple_ascii.json", PLAIN_JSON_DONE },
    { "y_string_space.json", PLAIN_JSON_DONE },
    { "y_string_surrogates_U+1D11E_MUSICAL_SYMBOL_G_CLEF.json", PLAIN_JSON_DONE },
    { "y_string_three-byte-utf-8.json", PLAIN_JSON_DONE },
    { "y_string_two-byte-utf-8.json", PLAIN_JSON_DONE },
    { "y_string_u+2028_line_sep.json", PLAIN_JSON_DONE },
    { "y_string_u+2029_par_sep.json", PLAIN_JSON_DONE },
    { "y_string_uEscape.json", PLAIN_JSON_DONE },
    { "y_string_uescaped_newline.json", PLAIN_JSON_DONE },
    { "y_string_unescaped_char_delete.json", PLAIN_JSON_DONE },
    { "y_string_unicode.json", PLAIN_JSON_DONE },
    { "y_string_unicodeEscapedBackslash.json", PLAIN_JSON_DONE },
    { "y_string_unicode_2.json", PLAIN_JSON_DONE },
    { "y_string_unicode_U+10FFFE_nonchar.json", PLAIN_JSON_DONE },
    { "y_string_unicode_U+1FFFE_nonchar.json", PLAIN_JSON_DONE },
    { "y_string_unicode_U+200B_ZERO_WIDTH_SPACE.json", PLAIN_JSON_DONE },
    { "y_string_unicode_U+2064_invisible_plus.json", PLAIN_JSON_DONE },
    { "y_string_unicode_U+FDD0_nonchar.json", PLAIN_JSON_DONE },
    { "y_string_unicode_U+FFFE_nonchar.json", PLAIN_JSON_DONE },
    { "y_string_unicode_escaped_double_quote.json", PLAIN_JSON_DONE },
    { "y_string_utf8.json", PLAIN_JSON_DONE },
    { "y_string_with_del_character.json", PLAIN_JSON_DONE },
    { "y_structure_lonely_false.json", PLAIN_JSON_DONE },
    { "y_structure_lonely_int.json", PLAIN_JSON_DONE },
    { "y_structure_lonely_negative_real.json", PLAIN_JSON_DONE },
    { "y_structure_lonely_null.json", PLAIN_JSON_DONE },
    { "y_structure_lonely_string.json", PLAIN_JSON_DONE },
    { "y_structure_lonely_true.json", PLAIN_JSON_DONE },
    { "y_structure_string_empty.json", PLAIN_JSON_DONE },
    { "y_structure_trailing_newline.json", PLAIN_JSON_DONE },
    { "y_structure_true_in_array.json", PLAIN_JSON_DONE },
    { "y_structure_whitespace_array.json", PLAIN_JSON_DONE },
};

#define ANSI_DIM   "\033[2m"
#define ANSI_RESET "\033[0m"

#define ANSI_BLACK   "\033[30m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_DEFAULT "\033[38m"

#define PATH_BUFFER_SIZE  256
#define TOKEN_BUFFER_SIZE 64
int main(void) {
    const char *path = "data/JSONTestSuite/test_parsing/";
    char pathname[PATH_BUFFER_SIZE] = { 0 };
    memcpy(pathname, path, strlen(path));

    plain_json_Token *tokens = malloc(TOKEN_BUFFER_SIZE * sizeof(*tokens));

    int total_files = sizeof(test_parsing_cases) / sizeof(test_parsing_cases[0]);
    for (int i = 0; i < total_files; i++) {
        TestCase test_case = test_parsing_cases[i];
        memset(pathname + strlen(path), 0, PATH_BUFFER_SIZE - strlen(path));
        memcpy(pathname + strlen(path), test_case.filename, strlen(test_case.filename));

        FILE *text_file = fopen(pathname, "r");
        if (text_file == NULL) {
            perror(pathname);
            goto on_error;
        }

        fseek(text_file, 0, SEEK_END);
        long text_size = ftell(text_file);
        rewind(text_file);
        char *text_buffer = calloc(text_size + 1, 1);

        if (fread(text_buffer, text_size, 1, text_file) != 1 && text_size > 0) {
            perror(pathname);
            free(text_buffer);
            fclose(text_file);
            goto on_error;
        }

        plain_json_Context context = { 0 };
        plain_json_load_buffer(&context, text_buffer, text_size);

        plain_json_ErrorType result = PLAIN_JSON_HAS_REMAINING;
        int tokens_read = 0;
        result = plain_json_read_token_buffered(&context, tokens, TOKEN_BUFFER_SIZE, &tokens_read);

        int passed = 0;
        int report_string_index = 0;
        static const char *report_string[] = {
            ANSI_GREEN "SUCCESS" ANSI_RESET,          ANSI_RED "FAILURE" ANSI_RESET,
            ANSI_YELLOW "PARTIAL_FAILURE" ANSI_RESET, ANSI_BLUE "UNDEFINDED_SUCCESS" ANSI_RESET,
            ANSI_CYAN "UNDEFINED_FAILURE" ANSI_RESET,
        };

        switch (test_case.filename[0]) {
            // valid:
            //      - The test succeeded [SUCCESS]
            //      - The test failed    [FAILURE]
            // invalid:
            //      - The test succeeded [FAILURE]
            //      - The test failed and the error matche [SUCCESS]
            //      - The test failed and the error does not match [PARTIAL_FAILURE]
            // undefined:
            //      - The test succeeded
            //      - The test failed
        case 'i':
            if (test_case.expected_result == result) {
                report_string_index = 3;
                passed = 1;
            } else {
                report_string_index = 4;
            }
            break;
        case 'y':
            if (result == PLAIN_JSON_DONE) {
                report_string_index = 0;
                passed = 1;
            } else {
                report_string_index = 1;
            }
            break;
        case 'n':
            if (result == PLAIN_JSON_DONE) {
                report_string_index = 1;
            } else if (test_case.expected_result != result) {
                report_string_index = 2;
            } else {
                report_string_index = 0;
                passed = 1;
            }
            break;
        }

#ifndef PRINT_RESULT_LIST
        if (!PRINT_ONLY_FAILURE || !passed) {
            printf(
                "%02d. %-50s%-25s (got: '%s' expected: '%s')\n", i, test_case.filename,
                report_string[report_string_index], plain_json_error_to_string(result),
                plain_json_error_to_string(test_case.expected_result)
            );
            if (!passed) {
                printf("%s\n", text_buffer);
            }
        }
#else
        printf("{\"%s\", %s},\n", test_case.filename, plain_json_error_to_string(result));
#endif

        free(text_buffer);
        fclose(text_file);
    }

    free(tokens);
    return 0;
on_error:
    free(tokens);
    return 1;
}
