/**
 * \mainpage VCS TCP/IP bulletin board - server business logic
 *
 * In the course "Verteilte Computer Systeme" the students shall
 * implement a bulletin board. It shall consist of a spawning TCP/IP
 * server (\a simple_message_server(1)) which executes the business logic
 * (\a simple_message_server_logic(1)) provided by the lector,
 * and a suitable TCP/IP client (\a simple_message_client(1)).
 *
 * The following files are part of the distribution:
 *
 * <dl>
 * <dt>README.txt</dt>
 * <dd>
 *     This file. - Provides an overview of the server business logic
 * </dd>
 * <dt>Makefile</dt>
 * <dd>
 *     The Makefile used to build the project and to install the
 *     executables and the manpages on annuminas.
 * </dd>
 * <dt>bin2c.c</dt>
 * <dd>
 *     An utility to convert a binary file into a C source vector.
       Source: <a href="http://wiki.wxwidgets.org/Embedding_PNG_Images-Bin2c_In_C">http://wiki.wxwidgets.org/Embedding_PNG_Images-Bin2c_In_C</a>
 * </dd>
 * <dt>simple_message_server_logic.c</dt>
 * <dd>
 *     The business logic of the bulletin board (see
 *     simple_message_server_logic(1)). It creates
 *     <code>~/public_html/vcs_tcpip_bulletin_board.php</code> at
 *     startup if the file does not exits and writes the data given by
 *     the client into the file
 *     <code>bulletin_board_content.dat</code> which is also located
 *     in the user's <code>public_html</code> directory.  The program
 *     can be instructed to perform several test cases by setting the
 *     environment variable <code>SMSL_TESTCASE</code>. Please invoke
 *     <code>simple_message_server_logic --help</code> for detailed
 *     information.
 * </dd>
 * <dt>simple_message_server_logic.1</dt>
 * <dd>
 *     The manual page for business logic of the spawning server
 *     (see simple_message_server_logic(1)).
 * </dd>
 * <dt>vcs_tcpip_bulletin_board.php</dt>
 * <dd>
 *      Dynamic HTLM page implementing the web front-end of the
 *      bulletin board. It includes the file
 *      <code>bulletin_board_content.dat</code> which is created by
 *      <code>simple_message_server_logic</code>.
 * </dd>
 * <dt>
 *     content_entry_with_img.thtml, content_entry_without_img.thtml,
 *     vcs_tcpip_bulletin_board_response_error.thtml,
 *     vcs_tcpip_bulletin_board_response_ok.thtml,
 *     vcs_tcpip_bulletin_board.php
 * </dt>
 * <dd>
 *     HTML fragments used as templates to construct the server
 *     response and to build the bulletin board web page. The build
 *     process converts this files into <code>&lt;filename&gt;.h</code>
 *     which are included into <code>simple_message_server_logic.c</code>.
 * </dd>
 * <dt>error.png, ok.png</dt>
 * <dd>
 *     Images used as part of the response server response. The build
 *     process converts this files into <code>error.png.h</code> and
 *     <code>ok.png.h</code> which are included into
 *     <code>simple_message_server_logic.c</code>.
 * </dd>
 * </dl>
 */
