from PIL import Image


def process_image(input_path, output_path, channel="red"):
    # Open the image
    img = Image.open(input_path).convert("RGB")
    pixels = img.load()

    # Channel index: 0 for red, 1 for green, 2 for blue
    channel_idx = {"red": 0, "green": 1, "blue": 2}[channel.lower()]

    # Process each pixel
    for y in range(img.height):
        for x in range(img.width):
            r, g, b = pixels[x, y]

            # Check if the pixel is totally white
            if r == g == b:
                continue  # Keep the original white pixel

            # Create a new pixel with only the selected channel
            channels = [0, 0, 0]
            channels[channel_idx] = [r, g, b][channel_idx]
            pixels[x, y] = tuple(channels)

    # Save the processed image
    img.save(output_path)
    print(f"Processed image saved as {output_path}")


# Example usage:
# Replace 'input.png' and 'output.png' with your file paths.
process_image("data/pacman_ghost.png", "data/pacman_ghost_green.png", channel="green")
