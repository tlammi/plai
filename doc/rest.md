# Plai REST API

All operations in the API are atomic.

## GET /_ping
Check whether the service is available
Always returns 200.

## GET /media/\[image|video\]/\<name\>
Get metadata about a media. The response body has format

Status Code:
Returns 200 on success and 404 if the media cannot be found.
Response Body:
Contains metadata as key-value pairs in JSON.

Fields:
- `size`: Size of the media in bytes
- `sha256`: SHA256 digest of the data as hex string
```JSON
{
  "size": 1234,
  "sha256": "deadbeef",
}
```

## PUT /media/\[image|video\]/\<name\>
Add a video or an image to the library.
Payload: binary data of the media.
Returns 201 if a new media was created and 200 if one was overwritten.

## DELETE /media\[image|video\]/\<name\>
Delete a video or an image from the library.
The change might take effect later if the media is currently being played.
On success, returns 200 or 202 depending whether the deletion is performed immediately or scheduled to be done later.
On failure returns 4xx.

## GET /media/\[image|video\]
Get list of images or videos.
Returns a JSON array where each element is a string and has format `"[image|video]/<name>"`.

## GET /media
Get list of both images and videos. Same as requesting both GET /media/image and GET /media/video

## POST /play?replay=true
Body: JSON with list of media paths to play.
Media paths have format `[image|video]/<name>`
Body:
```JSON
["video/media1", "image/media2", ...]
```

Parameters:
- `replay`: `true/false`
    - Wether to replay the medias from start after all are played. Default is true

Status Code:
Returns 200 if the player was started successfully and 404 if any of the medias was not found.

