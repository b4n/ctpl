
namespace Ctpl {
	[CCode (ref_function = "ctpl_environ_ref", unref_function = "ctpl_environ_unref", type_id = "ctpl_environ_get_type ()")]
	[Compact]
	public class Environ {
		/* Fix buggy argument type (see .metadata) */
		[CCode (cname = "ctpl_environ_push_gvalue_parray")]
		public bool push_parray (string symbol, GLib.Value?[] value) throws GLib.Error;

		/* These don't actually work, vapigen strips the function bodies... */
		[CCode (cname = "_ctpl_vala_environ_push_garray")]
		public bool push_garray (string symbol, GLib.Array<GLib.Value?> value) throws GLib.Error {
			push_parray(symbol, value.data);
		}
		[CCode (cname = "_ctpl_vala_environ_push_generic_array")]
		public bool push_generic_array (string symbol, GLib.GenericArray<GLib.Value?> value) throws GLib.Error {
			push_parray(symbol, value.data);
		}
	}

	namespace Lexer {}
	namespace Parser {}

	[CCode (copy_function = "g_boxed_copy", free_function = "g_boxed_free", type_id = "ctpl_token_get_type ()")]
	[Compact]
	public class Token {
		/* extra convenient API, reflects Ctpl.Lexer */
		[CCode (has_construct_function = false, cname = "ctpl_lexer_lex_gstream")]
		public Token.from_stream (GLib.InputStream gstream, string? name = null) throws GLib.Error;
		[CCode (has_construct_function = false, cname = "ctpl_lexer_lex_path")]
		public Token.from_path (string path) throws GLib.Error;
		[CCode (has_construct_function = false, cname = "ctpl_lexer_lex_string")]
		public Token.from_string (string template) throws GLib.Error;
		/* extra convenient API, reflects Ctpl.Parser */
		[CCode (cname = "ctpl_parser_parse_to_gstream")]
		public bool parse (Ctpl.Environ env, GLib.OutputStream output) throws GLib.Error;
	}

	/* for some reasons those are missing from the GIR */
	[CCode (cname = "ctpl_major_version")]
	public const uint major_version;
	[CCode (cname = "ctpl_minor_version")]
	public const uint minor_version;
	[CCode (cname = "ctpl_micro_version")]
	public const uint micro_version;
}
