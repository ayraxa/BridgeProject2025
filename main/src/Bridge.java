package main.src;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;
import java.util.logging.Logger;

public class Bridge {

    private static final Logger logger = Logger.getLogger(Bridge.class.getName());
    private static final String ESP_IP   = "172.20.10.4";
    private static final int    ESP_PORT = 1234;  

    public static void main(String[] args) throws IOException {
        startUiServer();   // never returns
    }

    // starts http server on localhost:1234 that listens for POST 
    private static void startUiServer() throws IOException {
        int uiPort = 1234;                                  // matches fetch() in index.html
        HttpServer server = HttpServer.create(new InetSocketAddress(uiPort), 0);
        server.createContext("/command", new CommandHandler());

        //cached thread pool 
        server.setExecutor(Executors.newCachedThreadPool());

        server.start();
        logger.info("Bridge UI server listening on http://localhost:" + uiPort + "/command");
    }

    // Handles POST /command from the HTML UI.
    private static class CommandHandler implements HttpHandler {
        @Override public void handle(HttpExchange exchange) throws IOException {
            if (!"POST".equals(exchange.getRequestMethod())) {
                exchange.sendResponseHeaders(405, -1);      // method n/a
                return;
            }

            // read
            String command;
            try (InputStream in = exchange.getRequestBody()) {
                command = new String(in.readAllBytes(), StandardCharsets.UTF_8).trim();
            }
            logger.info("[UI] received command: " + command);

            // give to esp32
            try (Socket sock = new Socket(ESP_IP, ESP_PORT);
                 PrintWriter out = new PrintWriter(sock.getOutputStream(), true)) {

                out.println(command);
                logger.info("[ESP]     forwarded: " + command);
            } catch (IOException io) {
                logger.severe("[ESP] connection failed: " + io.getMessage());
            }

            // reply 200 OK to the browser
            exchange.sendResponseHeaders(200, -1);
        }
    }
}
