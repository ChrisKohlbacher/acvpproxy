/* Reading and writing of ESVP status information for re-entrant support
 *
 * Copyright (C) 2021 - 2022, Stephan Mueller <smueller@chronox.de>
 *
 * License: see LICENSE file in root directory
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <string.h>

#include "esvp_internal.h"
#include "json_wrapper.h"

/***************************************************************************
 * ESVP status handling
 ***************************************************************************/
int esvp_read_status(const struct acvp_testid_ctx *testid_ctx,
		     struct json_object *status)
{
	struct esvp_es_def *es = testid_ctx->es_def;
	struct esvp_cc_def *cc;
	struct esvp_sd_def *sd = NULL;
	struct acvp_auth_ctx *auth;
	struct json_object *array;
	unsigned int seq_no = 1, i;
	const char *str;
	int ret;

	logger(LOGGER_DEBUG, LOGGER_C_ANY, "Parsing status of ESVP\n");

	CKINT(json_get_uint(status, "rawNoiseBitsId", &es->raw_noise_id));
	CKINT(json_get_bool(status, "rawNoiseBitsSubmitted",
			    &es->raw_noise_submitted));
	CKINT(json_get_uint(status, "restartTestBitsId", &es->restart_id));
	CKINT(json_get_bool(status, "restartTestBitsSubmitted",
			    &es->restart_submitted));

	/* Get access token */
	CKINT(json_get_string(status, "eaAccessToken", &str));
	/* Store access token in ctx */
	CKINT(acvp_init_acvp_auth_ctx(&es->es_auth));
	auth = es->es_auth;
	CKINT_LOG(acvp_set_authtoken_temp(auth, str),
		  "Cannot set the new JWT token\n");

	/* Duplicate the authtoken for the testid context */
	CKINT(acvp_copy_auth(testid_ctx->server_auth, auth));

	CKINT(json_get_uint64(status, "eaAccessTokenGenerated",
			      (uint64_t *)&auth->jwt_token_generated));

	for (cc = es->cc; cc; cc = cc->next, seq_no++) {
		struct json_object *stat_cc;
		char ref[40];

		/* Only process non-vetted conditioning components */
		if (cc->vetted)
			continue;

		snprintf(ref, sizeof(ref), "conditioningComponent%u", seq_no);
		CKINT(json_find_key(status, ref, &stat_cc, json_type_object));
		CKINT(json_get_uint(stat_cc, "conditionedBitsId", &cc->cc_id));
		CKINT(json_get_bool(stat_cc, "conditionedBitsSubmitted",
				    &cc->output_submitted));
	}

	ret = json_find_key(status, "supportingDocumentation", &array,
			    json_type_array);
	if (ret) {
		ret = 0;
		goto out;
	}

	for (i = 0; i < json_object_array_length(array); i++) {
		struct json_object *sd_entry =
			json_object_array_get_idx(array, i);

		sd = calloc(1, sizeof(struct esvp_sd_def));
		CKNULL(sd, -ENOMEM);

		/* Get access token */
		CKINT(json_get_string(sd_entry, "accessToken", &str));
		/* Store access token in ctx */
		CKINT(acvp_init_acvp_auth_ctx(&sd->sd_auth));
		auth = sd->sd_auth;
		CKINT_LOG(acvp_set_authtoken_temp(auth, str),
			  "Cannot set the new JWT token\n");

		CKINT(json_get_uint64(sd_entry, "accessTokenGenerated",
				      (uint64_t *)&auth->jwt_token_generated));

		CKINT(json_get_uint(sd_entry, "sdId", &sd->sd_id));
		CKINT(json_get_string(sd_entry, "filename", &str);
		      CKINT(acvp_duplicate(&sd->filename, str)));

		/*
		 * Append the new conditioning component entry at the end of the list
		 * because the order matters.
		 */
		if (es->sd) {
			struct esvp_sd_def *iter_sd = es->sd;

			while (iter_sd) {
				if (!iter_sd->next) {
					iter_sd->next = sd;
					break;
				}
				iter_sd = iter_sd->next;
			}
		} else {
			es->sd = sd;
		}
	}

out:
	if (ret)
		esvp_def_sd_free(sd);

	return ret;
}

int esvp_build_sd(const struct acvp_testid_ctx *testid_ctx,
		  struct json_object *data, bool write_extended)
{
	const struct esvp_es_def *es = testid_ctx->es_def;
	const struct esvp_sd_def *sd;
	struct json_object *sd_array;
	int ret;

	if (!es->sd)
		return 0;

	sd_array = json_object_new_array();
	CKINT(json_object_object_add(data, "supportingDocumentation",
				     sd_array));

	for (sd = es->sd; sd; sd = sd->next) {
		struct json_object *sd_data;
		struct acvp_auth_ctx *auth = sd->sd_auth;

		sd_data = json_object_new_object();
		CKNULL(sd_data, -ENOMEM);
		CKINT(json_object_array_add(sd_array, sd_data));
		CKINT(json_object_object_add(
			sd_data, "sdId", json_object_new_int((int)sd->sd_id)));
		CKINT(json_object_object_add(
			sd_data, "accessToken",
			json_object_new_string(auth->jwt_token)));
		if (write_extended) {
			CKINT(json_object_object_add(
				sd_data, "accessTokenGenerated",
				json_object_new_int64(
					auth->jwt_token_generated)));
			CKINT(json_object_object_add(
				sd_data, "filename",
				json_object_new_string(sd->filename)));
		}
	}

out:
	return ret;
}

int esvp_write_status(const struct acvp_testid_ctx *testid_ctx)
{
	const struct acvp_ctx *ctx = testid_ctx->ctx;
	const struct acvp_datastore_ctx *datastore = &ctx->datastore;
	const struct esvp_es_def *es = testid_ctx->es_def;
	const struct esvp_cc_def *cc;
	struct json_object *stat = NULL;
	struct acvp_buf stat_buf;
	struct acvp_auth_ctx *auth;
	const char *stat_str;
	unsigned int seq_no = 1;
	int ret;

	stat = json_object_new_object();
	CKNULL(stat, -ENOMEM);

	CKINT(json_object_object_add(
		stat, "rawNoiseBitsId",
		json_object_new_int((int)es->raw_noise_id)));
	CKINT(json_object_object_add(
		stat, "rawNoiseBitsSubmitted",
		json_object_new_boolean(es->raw_noise_submitted)));
	CKINT(json_object_object_add(stat, "restartTestBitsId",
				     json_object_new_int((int)es->restart_id)));
	CKINT(json_object_object_add(
		stat, "restartTestBitsSubmitted",
		json_object_new_boolean(es->restart_submitted)));

	auth = es->es_auth;
	if (auth) {
		CKINT(json_object_object_add(
			stat, "eaAccessToken",
			json_object_new_string(auth->jwt_token)));
		CKINT(json_object_object_add(
			stat, "eaAccessTokenGenerated",
			json_object_new_int64(auth->jwt_token_generated)));
	}

	for (cc = es->cc; cc; cc = cc->next, seq_no++) {
		struct json_object *stat_cc;
		char ref[40];

		/* Only process non-vetted conditioning components */
		if (cc->vetted)
			continue;

		stat_cc = json_object_new_object();
		CKNULL(stat_cc, -ENOMEM);

		snprintf(ref, sizeof(ref), "conditioningComponent%u", seq_no);

		CKINT(json_object_object_add(stat_cc, ref, stat_cc));
		CKINT(json_object_object_add(
			stat_cc, "conditionedBitsId",
			json_object_new_int((int)cc->cc_id)));
		CKINT(json_object_object_add(
			stat_cc, "conditionedBitsSubmitted",
			json_object_new_boolean(cc->output_submitted)));
	}

	CKINT(esvp_build_sd(testid_ctx, stat, true));

	stat_str = json_object_to_json_string_ext(
		stat, JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE);
	CKNULL_LOG(stat_str, -ENOMEM,
		   "JSON object conversion into string failed\n");

	stat_buf.buf = (uint8_t *)stat_str;
	stat_buf.len = (uint32_t)strlen(stat_str);

	/* Store the testID meta data */
	CKINT(ds->acvp_datastore_write_testid(
		testid_ctx, datastore->esvp_statusfile, true, &stat_buf));

out:
	ACVP_JSON_PUT_NULL(stat);

	return ret;
}
