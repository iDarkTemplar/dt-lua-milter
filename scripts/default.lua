function parse_sender(sender_string)
	local parsed_sender, parsed_subaddress, parsed_domain
	local remaining_string = sender_string

	-- first parse sender address
	local i, k = string.find(remaining_string, "[+@]");
	
	if i ~= nil then
		parsed_sender = string.sub(remaining_string, 1, i - 1)

		local found_sign = string.sub(remaining_string, i, k)

		remaining_string = string.sub(remaining_string, i + 1)

		if found_sign == "@" then
			-- only domain remains
			i, k = string.find(remaining_string, "[+@]");

			if i ~= nil then
				return false -- domain shouldn't contain '+' or '@' symbols
			end

			parsed_domain = remaining_string

			if string.len(parsed_domain) == 0 then
				return false
			end
		else
			-- subaddress remains, maybe with domain
			i, k = string.find(remaining_string, "[+@]")

			if i ~= nil then
				found_sign = string.sub(remaining_string, i, k)

				if found_sign ~= "@" then
					return false -- it shouldn't contain multiple '+' symbols
				end

				-- subaddress + domain
				parsed_subaddress = string.sub(remaining_string, 1, i - 1)

				remaining_string = string.sub(remaining_string, i + 1)

				-- only domain remains
				i, k = string.find(remaining_string, "[+@]");

				if i ~= nil then
					return false -- domain shouldn't contain '+' or '@' symbols
				end

				parsed_domain = remaining_string

				if string.len(parsed_domain) == 0 then
					return false
				end
			else
				-- no domain specified, only subaddress
				parsed_subaddress = remaining_string
			end
			
			if string.len(parsed_subaddress) == 0 then
				return false
			end
		end
	else
		-- only sender address present
		parsed_sender = remaining_string
	end
	
	if string.len(parsed_sender) == 0 then
		return false
	end

	return true, parsed_sender, parsed_subaddress, parsed_domain
end

-- All callbacks are below

function mlfi_connect(hostname)
    return SMFIS_CONTINUE
end

function mlfi_helo(helohost)
    return SMFIS_CONTINUE
end

function mlfi_envfrom(...)
    headers = {} -- clean headers
    sender = smfi_getsymval("{mail_addr}")
    return SMFIS_CONTINUE
end

function mlfi_envrcpt(...)
    return SMFIS_CONTINUE
end

function mlfi_header(header, header_value)
	if string.lower(header) == "from" then
		local envelope_sender = string.lower(sender)
		local header_sender = string.lower(header_value)
		local parsed_ok, parsed_user, parsed_subaddress, parsed_domain
		
		if not parsed_ok then
			smfi_setreply("550", nil, "failed to parse \"from\" header: " .. header_value)
			return SMFIS_REJECT
		end

		-- TODO: allow users without domain, check users map
		if parsed_user .. "@" .. parsed_domain ~= envelope_sender then
			--smfi_setreply("550", nil, "your user is not allowed to change \"from\" header to: " .. header_value)
			smfi_setreply("550", nil, "your user is not allowed to change \"from\" header to: " .. header_value .. ", envelope sender: " .. sender)
		end
    end

    return SMFIS_CONTINUE
end

function mlfi_eoh()
    return SMFIS_CONTINUE
end

function mlfi_body(body, body_len)
    return SMFIS_CONTINUE
end

function mlfi_eom()
    return SMFIS_CONTINUE
end

function mlfi_abort()
    return SMFIS_CONTINUE
end

function mlfi_close()
    return SMFIS_CONTINUE
end

function mlfi_unknown(cmd)
    return SMFIS_CONTINUE
end

function mlfi_data()
    return SMFIS_CONTINUE
end
