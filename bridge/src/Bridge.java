package bridge.src;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.logging.Logger;

public class Bridge {
    private static final Logger logger = Logger.getLogger(Bridge.class.getName());

    public static void main() {
        final int port = 1234; // FIX

        try (ServerSocket serverSocket = new ServerSocket(port)) {
            logger.info("bridge server is on port: " + port);

            while (true) {
                Socket socket = serverSocket.accept();
                logger.info("ESP32 connected: " + socket.getInetAddress());

                BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

                String inputLine;
                while ((inputLine = in.readLine()) != null) {
                    logger.info("received from ESP32: " + inputLine);
                }

                socket.close();
                logger.info("ESP32 disconnected.");
                break; 
            }

        } catch (Exception e) {
            logger.severe("error: " + e.getMessage());
        }
    }
}
