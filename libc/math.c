/* serie de taylor */
float sinf(float x) {
  float x2 = x * x;

  return x - (x2 * x) / 6.0f + (x2 * x2 * x) / 120.0f -
         (x2 * x2 * x2 * x) / 5040.0f;
}

float cosf(float x) {
  float x2 = x * x;

  return 1.0f - x2 / 2.0f + (x2 * x2) / 24.0f - (x2 * x2 * x2) / 720.0f;
}
